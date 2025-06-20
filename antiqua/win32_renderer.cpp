#include <d3d12.h>
#include <dxgi1_6.h>
#include <wrl.h>

#include "win32_antiqua.h"
#include "antiqua.h"
#include "antiqua_render_group.h"

#define CULLING_ENABLED 1
#define WIREFRAME_MODE_ENABLED 0

#define RECT_PIPELINE_STATE_IDX 0
#define LINE_PIPELINE_STATE_IDX 1
#define POINT_PIPELINE_STATE_IDX 2
#define TILE_PIPELINE_STATE_IDX 3
#define TEXTURE_DEBUG_PIPELINE_STATE_IDX 4

#define PIPELINE_STATE_COUNT 5

#define SWAP_CHAIN_BUFFER_COUNT 2

#define MAX_RENDER_GROUP_VB_SIZE KB(256)
#define MAX_RENDER_GROUP_PER_OBJECT_CB_SIZE KB(512)
#define MAX_RENDER_GROUP_PER_PASS_CB_SIZE KB(1)

#define ThrowIfFailed(hr) ASSERT(SUCCEEDED((hr)))

#define ENABLE_VSYNC 1

#define MAX_D3D_RESET_ATTEMPTS 3
u32 currentD3DResetAttempt = 0;

State *win32State = NULL;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

internal b32 useSoftwareRenderer = false;

internal D3D12_VIEWPORT viewport = {};
internal D3D12_RECT scissorRect;

b32 TearingSupportEnabled;

// Pipeline objects.
ComPtr<IDXGISwapChain3> swapChain = NULL;

internal ComPtr<ID3D12Device> device;
internal ComPtr<ID3D12Resource> renderTargets[SWAP_CHAIN_BUFFER_COUNT];
internal ComPtr<ID3D12CommandAllocator> commandAllocators[SWAP_CHAIN_BUFFER_COUNT];
internal ComPtr<ID3D12CommandQueue> commandQueue;
internal ComPtr<ID3D12DescriptorHeap> rtvHeap;
internal ComPtr<ID3D12PipelineState> pipelineState;
internal ComPtr<ID3D12GraphicsCommandList> commandList;
internal u32 rtvDescriptorSize;

// Synchronization objects.
internal u32 frameIndex;
internal HANDLE fenceEvent;
internal ComPtr<ID3D12Fence> fence;
internal u64 fenceValues[SWAP_CHAIN_BUFFER_COUNT];

internal void GetHardwareAdapter(
    IDXGIFactory1* pFactory,
    IDXGIAdapter1** ppAdapter,
    b32 requestHighPerformanceAdapter)
{
    *ppAdapter = NULL;

    ComPtr<IDXGIAdapter1> adapter;

    ComPtr<IDXGIFactory6> factory6;
    if (SUCCEEDED(pFactory->QueryInterface(IID_PPV_ARGS(&factory6))))
    {
        for (
            u32 adapterIndex = 0;
            SUCCEEDED(factory6->EnumAdapterByGpuPreference(
                adapterIndex,
                requestHighPerformanceAdapter == true ? DXGI_GPU_PREFERENCE_HIGH_PERFORMANCE : DXGI_GPU_PREFERENCE_UNSPECIFIED,
                IID_PPV_ARGS(&adapter)));
            ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), NULL)))
            {
                break;
            }
        }
    }

    if(adapter.Get() == NULL)
    {
        for (u32 adapterIndex = 0; SUCCEEDED(pFactory->EnumAdapters1(adapterIndex, &adapter)); ++adapterIndex)
        {
            DXGI_ADAPTER_DESC1 desc;
            adapter->GetDesc1(&desc);

            if (desc.Flags & DXGI_ADAPTER_FLAG_SOFTWARE)
            {
                // Don't select the Basic Render Driver adapter.
                // If you want a software adapter, pass in "/warp" on the command line.
                continue;
            }

            // Check to see whether the adapter supports Direct3D 12, but don't create the
            // actual device yet.
            if (SUCCEEDED(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, _uuidof(ID3D12Device), NULL)))
            {
                break;
            }
        }
    }
    
    *ppAdapter = adapter.Detach();
}

internal inline D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(ID3D12DescriptorHeap *Heap, u32 DescriptorSize, u32 DescriptorIdx)
{
    D3D12_CPU_DESCRIPTOR_HANDLE StartHndl = Heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE CurrHndl;
    CurrHndl.ptr = StartHndl.ptr + DescriptorIdx*DescriptorSize;

    return CurrHndl;
}

internal inline D3D12_RESOURCE_BARRIER
GetResourceTransitionBarrier(ID3D12Resource *Resource, D3D12_RESOURCE_STATES Before, D3D12_RESOURCE_STATES After)
{
    D3D12_RESOURCE_TRANSITION_BARRIER TransitionBarrier = {};
    TransitionBarrier.pResource = Resource;
    TransitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    TransitionBarrier.StateBefore = Before;
    TransitionBarrier.StateAfter = After;

    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition = TransitionBarrier;

    return Barrier;
}

// Wait for pending GPU work to complete.
internal void WaitForGpu()
{
    // Schedule a Signal command in the queue.
    HRESULT HR = commandQueue->Signal(fence.Get(), fenceValues[frameIndex]);
    ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValues[frameIndex]));

    // Wait until the fence has been processed.
    ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent));
    WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);

    // Increment the fence value for the current frame.
    fenceValues[frameIndex]++;
}

RESIZE_WINDOW(ResizeWindow)
{
    // Determine if the swap buffers and other resources need to be resized or not.
    if ((width != win32State->windowWidth || height != win32State->windowHeight) && !minimized)
    {
        // Flush all current GPU commands.
        WaitForGpu();

        // Release the resources holding references to the swap chain (requirement of
        // IDXGISwapChain::ResizeBuffers) and reset the frame fence values to the
        // current fence value.
        for (u32 n = 0; n < SWAP_CHAIN_BUFFER_COUNT; n++)
        {
            renderTargets[n].Reset();
            fenceValues[n] = fenceValues[frameIndex];
        }

        // Resize the swap chain to the desired dimensions.
        DXGI_SWAP_CHAIN_DESC desc = {};
        swapChain->GetDesc(&desc);
        ThrowIfFailed(swapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, width, height, desc.BufferDesc.Format, desc.Flags));

        BOOL fullscreenState;
        ThrowIfFailed(swapChain->GetFullscreenState(&fullscreenState, NULL));
        win32State->windowedMode = !fullscreenState;

        // Reset the frame index to the current back buffer index.
        frameIndex = swapChain->GetCurrentBackBufferIndex();

        // Update for size change: Update the width, height, and aspect ratio member variables.
        {
            win32State->windowWidth = width;
            win32State->windowHeight = height;
        }

        // Load size dependent resources
        {
            // Update post view and scissor
            {
                viewport.TopLeftX = 0.0f;
                viewport.TopLeftY = 0.0f;
                viewport.Width = (r32)width;
                viewport.Height = (r32)height;
                viewport.MinDepth = 0.0f;
                viewport.MaxDepth = 1.0f;

                scissorRect = {0, 0, (s32)width, (s32)height};
            }

            // Create frame resources.
            {
                // Create a RTV for each frame.
                for (u32 n = 0; n < SWAP_CHAIN_BUFFER_COUNT; n++)
                {
                    D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvHeap.Get(),
                                                                                   rtvDescriptorSize,
                                                                                   n);
                    ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
                    device->CreateRenderTargetView(renderTargets[n].Get(), NULL, rtvHandle);
                }
            }

            // This is where you would create/resize intermediate render targets, depth stencils, or other resources
            // dependent on the window size.
        }
    }

    win32State->windowVisible = !minimized;
}

INIT_RENDERER(initRenderer)
{
    if (data)
    {
        win32State = (State *)data;
    }

    u32 dxgiFactoryFlags = 0;

#if ANTIQUA_INTERNAL
    // Enable the debug layer (requires the Graphics Tools "optional feature").
    // NOTE: Enabling the debug layer after device creation will invalidate the active device.
    {
        ComPtr<ID3D12Debug> debugController;
        if (SUCCEEDED(D3D12GetDebugInterface(IID_PPV_ARGS(&debugController))))
        {
            debugController->EnableDebugLayer();

            // Enable additional debug layers.
            dxgiFactoryFlags |= DXGI_CREATE_FACTORY_DEBUG;
        }
    }
#endif

    ComPtr<IDXGIFactory4> factory;
    ThrowIfFailed(CreateDXGIFactory2(dxgiFactoryFlags, IID_PPV_ARGS(&factory)));

    if (useSoftwareRenderer)
    {
        ComPtr<IDXGIAdapter> warpAdapter;
        ThrowIfFailed(factory->EnumWarpAdapter(IID_PPV_ARGS(&warpAdapter)));

        ThrowIfFailed(D3D12CreateDevice(
            warpAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)
            ));
    }
    else
    {
        ComPtr<IDXGIAdapter1> hardwareAdapter;
        GetHardwareAdapter(factory.Get(), &hardwareAdapter, true);

        ThrowIfFailed(D3D12CreateDevice(
            hardwareAdapter.Get(),
            D3D_FEATURE_LEVEL_11_0,
            IID_PPV_ARGS(&device)
            ));
    }

    // Describe and create the command queue.
    D3D12_COMMAND_QUEUE_DESC queueDesc = {};
    queueDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
    queueDesc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;

    ThrowIfFailed(device->CreateCommandQueue(&queueDesc, IID_PPV_ARGS(&commandQueue)));

    // Check tearing support
    {
        ComPtr<IDXGIFactory6> tearingSupportCheckFactory;
        HRESULT hr = CreateDXGIFactory1(IID_PPV_ARGS(&tearingSupportCheckFactory));
        b32 allowTearing = FALSE;
        if (SUCCEEDED(hr))
        {
            hr = tearingSupportCheckFactory->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
        }

        TearingSupportEnabled = SUCCEEDED(hr) && allowTearing;
    }

    // Describe and create the swap chain.
    DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
    swapChainDesc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
    swapChainDesc.Width = win32State->windowWidth;
    swapChainDesc.Height = win32State->windowHeight;
    swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
    swapChainDesc.SampleDesc.Count = 1;
    // It is recommended to always use the tearing flag when it is available.
    swapChainDesc.Flags = TearingSupportEnabled ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;

    ComPtr<IDXGISwapChain1> swapChainLocalVar;
    ThrowIfFailed(factory->CreateSwapChainForHwnd(
        commandQueue.Get(),        // Swap chain needs the queue so that it can force a flush on it.
        win32State->window,
        &swapChainDesc,
        NULL,
        NULL,
        &swapChainLocalVar
        ));

    // This sample does not support fullscreen transitions.
    ThrowIfFailed(factory->MakeWindowAssociation(win32State->window, DXGI_MWA_NO_ALT_ENTER));

    ThrowIfFailed(swapChainLocalVar.As(&swapChain));
    frameIndex = swapChain->GetCurrentBackBufferIndex();

    // Create descriptor heaps.
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create frame resources.
    {
        // Create a RTV for each frame.
        for (u32 n = 0; n < SWAP_CHAIN_BUFFER_COUNT; n++)
        {
            D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvHeap.Get(),
                                                                           rtvDescriptorSize,
                                                                           n);
            ThrowIfFailed(swapChain->GetBuffer(n, IID_PPV_ARGS(&renderTargets[n])));
            device->CreateRenderTargetView(renderTargets[n].Get(), NULL, rtvHandle);

            ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT,
                                                         IID_PPV_ARGS(&commandAllocators[n])));
        }
    }

    // Create the command list.
    ThrowIfFailed(device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, commandAllocators[frameIndex].Get(), NULL, IID_PPV_ARGS(&commandList)));

    // Command lists are created in the recording state, but there is nothing
    // to record yet. The main loop expects it to be closed, so close it now.
    ThrowIfFailed(commandList->Close());

    // Create synchronization objects.
    {
        ThrowIfFailed(device->CreateFence(fenceValues[frameIndex], D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));
        fenceValues[frameIndex]++;

        // Create an event handle to use for frame synchronization.
        fenceEvent = CreateEvent(NULL, FALSE, FALSE, NULL);
        if (fenceEvent == NULL)
        {
            ThrowIfFailed(HRESULT_FROM_WIN32(GetLastError()));
        }

        // Wait for the command list to execute; we are reusing the same command 
        // list in our main loop but for now, we just want to wait for setup to 
        // complete before continuing.
        WaitForGpu();
    }
}

RENDER_ON_GPU(renderOnGPU)
{
    if (!win32State->windowVisible)
    {
        return;
    }

    // Populate command list
    {
        // Command list allocators can only be reset when the associated 
        // command lists have finished execution on the GPU; apps should use 
        // fences to determine GPU execution progress.
        ThrowIfFailed(commandAllocators[frameIndex]->Reset());

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ThrowIfFailed(commandList->Reset(commandAllocators[frameIndex].Get(), pipelineState.Get()));

        // Indicate that the back buffer will be used as a render target.
        D3D12_RESOURCE_BARRIER Barrier = GetResourceTransitionBarrier(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &Barrier);

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvHeap.Get(),
                                                                       rtvDescriptorSize,
                                                                       frameIndex);

        // Record commands.
        r32 clearColor[] = { 0.0f, 0.2f, 0.4f, 1.0f };
        commandList->ClearRenderTargetView(rtvHandle, clearColor, 0, NULL);

        // Indicate that the back buffer will now be used to present.
        Barrier = GetResourceTransitionBarrier(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &Barrier);

        ThrowIfFailed(commandList->Close());
    }

    // Execute the command list.
#define COUNT_OF_COMMAND_LISTS 1
    ID3D12CommandList** ppCommandLists = PUSH_ARRAY(arena, COUNT_OF_COMMAND_LISTS, ID3D12CommandList *);
    ppCommandLists[0] = commandList.Get();
    commandQueue->ExecuteCommandLists(COUNT_OF_COMMAND_LISTS, ppCommandLists);
#undef COUNT_OF_COMMAND_LISTS

    // When using sync interval 0, it is recommended to always pass the tearing
    // flag when it is supported, even when presenting in windowed mode.
    // However, this flag cannot be used if the app is in fullscreen mode as a
    // result of calling SetFullscreenState.
    u32 presentFlags = (TearingSupportEnabled && win32State->windowedMode) ? DXGI_PRESENT_ALLOW_TEARING : 0;

    // Present the frame.
    HRESULT *HR = PUSH_STRUCT(arena, HRESULT);
    if (!ENABLE_VSYNC)
    {
        *HR = swapChain->Present(0, presentFlags);
    }
    else
    {
        *HR = swapChain->Present(1, 0);
    }
    if (!SUCCEEDED(*HR))
    {
        // NOTE(dima): fail hard if unable to reset D3D resources for several times in a row
        if (currentD3DResetAttempt++ >= MAX_D3D_RESET_ATTEMPTS)
        {
            ThrowIfFailed(*HR);
        }

        WaitForGpu();

        // Release D3D resources
        {
            fence.Reset();

            for (u32 n = 0; n < SWAP_CHAIN_BUFFER_COUNT; n++)
            {
                renderTargets[n].Reset();
            }

            commandQueue.Reset();
            swapChain.Reset();
            device.Reset();

            initRenderer(NULL);
        }
    }
    else
    {
        currentD3DResetAttempt = 0;
    }

    // Move to next frame
    {
        // Schedule a Signal command in the queue.
        u64 currentFenceValue = fenceValues[frameIndex];
        ThrowIfFailed(commandQueue->Signal(fence.Get(), currentFenceValue));

        // Update the frame index.
        frameIndex = swapChain->GetCurrentBackBufferIndex();

        // If the next frame is not ready to be rendered yet, wait until it is ready.
        if (fence->GetCompletedValue() < fenceValues[frameIndex])
        {
            ThrowIfFailed(fence->SetEventOnCompletion(fenceValues[frameIndex], fenceEvent));
            WaitForSingleObjectEx(fenceEvent, INFINITE, FALSE);
        }

        // Set the fence value for the next frame.
        fenceValues[frameIndex] = currentFenceValue + 1;
    }
}

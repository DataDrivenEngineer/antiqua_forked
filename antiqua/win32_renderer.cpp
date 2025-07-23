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
internal ComPtr<ID3D12Device> device = NULL;

ComPtr<IDXGISwapChain3> swapChain = NULL;
internal ComPtr<ID3D12Resource> renderTargets[SWAP_CHAIN_BUFFER_COUNT] = { NULL, NULL };

internal ComPtr<ID3D12Resource> depthStencilBuffer = NULL;

internal ComPtr<ID3D12RootSignature> rootSignature = NULL;

internal ComPtr<ID3D12PipelineState> pipelineState[PIPELINE_STATE_COUNT];

internal ComPtr<ID3D12CommandQueue> commandQueue = NULL;
internal ComPtr<ID3D12CommandAllocator> commandAllocators[SWAP_CHAIN_BUFFER_COUNT] = { NULL, NULL };
internal ComPtr<ID3D12GraphicsCommandList> commandList = NULL;

internal ComPtr<ID3D12DescriptorHeap> rtvHeap = NULL;
internal ComPtr<ID3D12DescriptorHeap> dsvHeap = NULL;
internal ComPtr<ID3D12DescriptorHeap> cbvSrvUavHeap[SWAP_CHAIN_BUFFER_COUNT];

internal u32 rtvDescriptorSize;
internal u32 dsvDescriptorSize;
internal u32 cbvSrvUavDescriptorSize;

internal ComPtr<ID3D12Resource> renderGroupPerPassCB = NULL;
internal ComPtr<ID3D12Resource> renderGroupPerObjectCB = NULL;

internal ComPtr<ID3D12Resource> renderGroupVb;

internal D3D12_RASTERIZER_DESC rasterizerDesc = {};
internal D3D12_BLEND_DESC blendDesc = {};

// Synchronization objects.
internal u32 frameIndex;
internal HANDLE fenceEvent;
internal ComPtr<ID3D12Fence> fence;
internal u64 fenceValues[SWAP_CHAIN_BUFFER_COUNT];

// NOTE(dima): application-specific fields
internal u32 renderGroupVBCurrentSize = 0;
internal u32 currentCbvSrvUavDescriptorIdx = 0;
internal u32 currentPerObjectCBOffset = 0;

internal AntiquaTexture texture[SWAP_CHAIN_BUFFER_COUNT] = {{}, {}};
internal ComPtr<ID3D12Resource> textureUploadHeap[SWAP_CHAIN_BUFFER_COUNT] = {NULL, NULL};

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

internal inline D3D12_HEAP_PROPERTIES
GetHeapProperties(D3D12_HEAP_TYPE type)
{
    D3D12_HEAP_PROPERTIES heapProperties = {};
    heapProperties.Type = type;
    heapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    heapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    heapProperties.CreationNodeMask = 1;
    heapProperties.VisibleNodeMask = 1;

    return heapProperties;
}

internal inline D3D12_RESOURCE_DESC
GetResourceDescriptor(D3D12_RESOURCE_DIMENSION dimension,
                      u64 alignment,
                      u64 width,
                      u32 height,
                      u16 depthOrArraySize,
                      u16 mipLevels,
                      DXGI_FORMAT format,
                      u32 sampleDescCount,
                      u32 sampleDescQuality,
                      D3D12_TEXTURE_LAYOUT layout,
                      D3D12_RESOURCE_FLAGS flags)
{
    D3D12_RESOURCE_DESC	desc = {};
    desc.Dimension = dimension;
    desc.Alignment = alignment;
    desc.Width = width;
    desc.Height = height;
    desc.DepthOrArraySize = depthOrArraySize;
    desc.MipLevels = mipLevels;
    desc.Format = format;
    desc.SampleDesc.Count = sampleDescCount;
    desc.SampleDesc.Quality = sampleDescQuality;
    desc.Layout = layout;
    desc.Flags = flags;

    return desc;
}

internal inline D3D12_RESOURCE_DESC
GetBufferResourceDescriptor(u64 width, D3D12_RESOURCE_FLAGS flags = D3D12_RESOURCE_FLAG_NONE, u64 alignment = 0)
{
    return GetResourceDescriptor(D3D12_RESOURCE_DIMENSION_BUFFER,
                                 alignment,
                                 width,
                                 1,
                                 1,
                                 1,
                                 DXGI_FORMAT_UNKNOWN,
                                 1,
                                 0,
                                 D3D12_TEXTURE_LAYOUT_ROW_MAJOR,
                                 flags);
}

internal inline D3D12_DESCRIPTOR_RANGE
GetDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE rangeType,
                   u32 numDescriptors,
                   u32 baseShaderRegister,
                   u32 registerSpace,
                   u32 offsetInDescriptorsFromTableStart)
{
    D3D12_DESCRIPTOR_RANGE range = {};
    range.RangeType = rangeType;
    range.NumDescriptors = numDescriptors;
    range.BaseShaderRegister = baseShaderRegister;
    range.RegisterSpace = registerSpace;
    range.OffsetInDescriptorsFromTableStart = offsetInDescriptorsFromTableStart;

    return range;
}

internal inline D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(ID3D12DescriptorHeap *heap,
                       u32 descriptorSize,
                       u32 descriptorIdx)
                       
                       
{
    D3D12_GPU_DESCRIPTOR_HANDLE startHndl = heap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE currHndl;
    currHndl.ptr = startHndl.ptr + descriptorIdx*descriptorSize;

    return currHndl;
}

internal inline D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(ID3D12DescriptorHeap *heap,
                       u32 descriptorSize,
                       u32 descriptorIdx)
{
    D3D12_CPU_DESCRIPTOR_HANDLE startHndl = heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE currHndl;
    currHndl.ptr = startHndl.ptr + descriptorIdx*descriptorSize;

    return currHndl;
}

internal inline void
CreateBuffer(u32 size, ID3D12Resource **buffer)
{
    D3D12_RESOURCE_DESC	desc = GetBufferResourceDescriptor(size);

    D3D12_HEAP_PROPERTIES heapProperties = GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &desc,
                                                  D3D12_RESOURCE_STATE_GENERIC_READ,
                                                  NULL,
                                                  IID_PPV_ARGS(buffer)));
}

internal inline void
CreateConstantBufferView(ID3D12Resource *buffer,
                         ID3D12DescriptorHeap *heap,
                         u32 descriptorSize,
                         u32 descriptorIdx,
                         u32 currentCBOffset,
                         u32 sizeOfDataRoundedToNearest256)
{
    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = buffer->GetGPUVirtualAddress() + currentCBOffset;
    D3D12_CPU_DESCRIPTOR_HANDLE currHndl = GetCPUDescriptorHandle(heap,
                                                                  descriptorSize,
                                                                  descriptorIdx);

    D3D12_CONSTANT_BUFFER_VIEW_DESC desc;
    desc.BufferLocation = cbAddress;
    desc.SizeInBytes = sizeOfDataRoundedToNearest256;

    device->CreateConstantBufferView(&desc, currHndl);
}

internal inline D3D12_RESOURCE_BARRIER
GetResourceTransitionBarrier(ID3D12Resource *resource,
                             D3D12_RESOURCE_STATES before,
                             D3D12_RESOURCE_STATES after)
{
    D3D12_RESOURCE_TRANSITION_BARRIER transitionBarrier = {};
    transitionBarrier.pResource = resource;
    transitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    transitionBarrier.StateBefore = before;
    transitionBarrier.StateAfter = after;

    D3D12_RESOURCE_BARRIER barrier = {};
    barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    barrier.Transition = transitionBarrier;

    return barrier;
}

// Wait for pending GPU work to complete.
internal inline void WaitForGpu()
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

internal inline void CreateDepthStencilBuffer(s32 windowWidth, s32 windowHeight)
{
    depthStencilBuffer.Reset();

    D3D12_RESOURCE_DESC	depthStencilDesc = GetResourceDescriptor(D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                                 0,
                                                                 windowWidth,
                                                                 windowHeight,
                                                                 1,
                                                                 1,
                                                                 DXGI_FORMAT_D24_UNORM_S8_UINT,
                                                                 1,
                                                                 0,
                                                                 D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                                                 D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);

    D3D12_CLEAR_VALUE clear = {};
    clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    clear.DepthStencil.Depth = 1.0f;
    clear.DepthStencil.Stencil = 0;


    D3D12_HEAP_PROPERTIES heapProperties = GetHeapProperties(D3D12_HEAP_TYPE_DEFAULT);

    ThrowIfFailed(device->CreateCommittedResource(&heapProperties,
                                                  D3D12_HEAP_FLAG_NONE,
                                                  &depthStencilDesc,
                                                  D3D12_RESOURCE_STATE_COMMON,
                                                  &clear,
                                                  IID_PPV_ARGS(depthStencilBuffer.ReleaseAndGetAddressOf())));

    D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc = {};
    dsvDesc.Flags = D3D12_DSV_FLAG_NONE;
	dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsvDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	dsvDesc.Texture2D.MipSlice = 0;

    device->CreateDepthStencilView(depthStencilBuffer.Get(),
                                   &dsvDesc,
                                   dsvHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_RESOURCE_BARRIER barrier = GetResourceTransitionBarrier(depthStencilBuffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE);
    commandList->ResourceBarrier(1, &barrier);
}

RESIZE_WINDOW(ResizeWindow)
{
    // Determine if the swap buffers and other resources need to be resized or not.
    if ((width != win32State->windowWidth || height != win32State->windowHeight) && !minimized)
    {
        // Flush all current GPU commands.
        WaitForGpu();

        ThrowIfFailed(commandList->Reset(commandAllocators[frameIndex].Get(), NULL));

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

            CreateDepthStencilBuffer(width, height);
        }

        ThrowIfFailed(commandList->Close());
#define COMMAND_LIST_COUNT 1
        ID3D12CommandList *ppCommandLists[] = { commandList.Get() };
        commandQueue->ExecuteCommandLists(COMMAND_LIST_COUNT, ppCommandLists);
#undef COMMAND_LIST_COUNT

        WaitForGpu();
    }

    win32State->windowVisible = !minimized;
}

INIT_RENDERER(initRenderer)
{
    if (data)
    {
        win32State = (State *)data;
    }

    ASSERT(win32State);

    GameMemory *gameMemory = (GameMemory *)win32State->gameMemory;

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

    // Create RTV descriptor heap
    {
        // Describe and create a render target view (RTV) descriptor heap.
        D3D12_DESCRIPTOR_HEAP_DESC rtvHeapDesc = {};
        rtvHeapDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
        rtvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        rtvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        ThrowIfFailed(device->CreateDescriptorHeap(&rtvHeapDesc, IID_PPV_ARGS(&rtvHeap)));

        rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
    }

    // Create DSV descriptor heap
    {
        D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
        dsvHeapDesc.NumDescriptors = 1;
        dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        dsvHeapDesc.NodeMask = 0;

        ThrowIfFailed(device->CreateDescriptorHeap(&dsvHeapDesc, IID_PPV_ARGS(dsvHeap.ReleaseAndGetAddressOf())));

        dsvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
    }

    // NOTE(dima): create CBV/SRV/UAV Descriptor Heap
    {
        // NOTE(dima): descriptor per renderable object + 1 for per pass descriptor + 1 for SRV descriptor
        // TODO(dima): do not hardcode max renderable object count!
        u32 maxDescriptorCount = SWAP_CHAIN_BUFFER_COUNT*(256 + 1 + 1);

        D3D12_DESCRIPTOR_HEAP_DESC desc = {};
        desc.NumDescriptors = maxDescriptorCount;
        desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        desc.NodeMask = 0;

        for (u32 n = 0; n < SWAP_CHAIN_BUFFER_COUNT; n++)
        {
            ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(cbvSrvUavHeap[n].ReleaseAndGetAddressOf())));
        }

        cbvSrvUavDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
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

    // NOTE(dima): create VB
    CreateBuffer(MAX_RENDER_GROUP_VB_SIZE,
                 renderGroupVb.ReleaseAndGetAddressOf());

    // NOTE(dima): create root signature
    {
        D3D12_DESCRIPTOR_RANGE perPassRange = GetDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

        D3D12_DESCRIPTOR_RANGE perObjectRange = GetDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 1, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

        D3D12_DESCRIPTOR_RANGE srvRange = GetDescriptorRange(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 1, 0, 0, D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND);

        D3D12_ROOT_DESCRIPTOR_TABLE perPassRootDescriptorTable = {};
        perPassRootDescriptorTable.NumDescriptorRanges = 1;
        perPassRootDescriptorTable.pDescriptorRanges = &perPassRange;

        D3D12_ROOT_DESCRIPTOR_TABLE perObjectRootDescriptorTable = {};
        perObjectRootDescriptorTable.NumDescriptorRanges = 1;
        perObjectRootDescriptorTable.pDescriptorRanges = &perObjectRange;

        D3D12_ROOT_DESCRIPTOR_TABLE srvRootDescriptorTable = {};
        srvRootDescriptorTable.NumDescriptorRanges = 1;
        srvRootDescriptorTable.pDescriptorRanges = &srvRange;

        D3D12_ROOT_DESCRIPTOR tileRootDescriptor;
        tileRootDescriptor.ShaderRegister = 2;
        tileRootDescriptor.RegisterSpace = 0;

        D3D12_ROOT_PARAMETER slotRootParameters[4];
        slotRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        slotRootParameters[0].DescriptorTable = perPassRootDescriptorTable;
        slotRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        slotRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        slotRootParameters[1].DescriptorTable = perObjectRootDescriptorTable;
        slotRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        slotRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        slotRootParameters[2].Descriptor = tileRootDescriptor;
        slotRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        slotRootParameters[3].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        slotRootParameters[3].DescriptorTable = srvRootDescriptorTable;
        slotRootParameters[3].ShaderVisibility = D3D12_SHADER_VISIBILITY_PIXEL;

        D3D12_STATIC_SAMPLER_DESC sampler = {};
        sampler.Filter = D3D12_FILTER_MIN_MAG_MIP_POINT;
        sampler.AddressU = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressV = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.AddressW = D3D12_TEXTURE_ADDRESS_MODE_BORDER;
        sampler.MipLODBias = 0;
        sampler.MaxAnisotropy = 16;
        sampler.ComparisonFunc = D3D12_COMPARISON_FUNC_LESS_EQUAL;
        sampler.BorderColor = D3D12_STATIC_BORDER_COLOR_OPAQUE_WHITE;
        sampler.MinLOD = 0.0f;
        sampler.MaxLOD = D3D12_FLOAT32_MAX;
        sampler.ShaderRegister = 0;
        sampler.RegisterSpace = 0;
        sampler.ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_ROOT_SIGNATURE_DESC rootSignatureDesc = {};
        rootSignatureDesc.NumParameters = 4;
        rootSignatureDesc.pParameters = slotRootParameters;
        rootSignatureDesc.NumStaticSamplers = 1;
        rootSignatureDesc.pStaticSamplers = &sampler;
        rootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        ComPtr<ID3DBlob> serializedRootSignature;
        ThrowIfFailed(D3D12SerializeRootSignature(&rootSignatureDesc,
                                                  D3D_ROOT_SIGNATURE_VERSION_1,
                                                  serializedRootSignature.ReleaseAndGetAddressOf(),
                                                  NULL));

        ThrowIfFailed(device->CreateRootSignature(0,
                                                  serializedRootSignature->GetBufferPointer(),
                                                  serializedRootSignature->GetBufferSize(),
                                                  IID_PPV_ARGS(rootSignature.ReleaseAndGetAddressOf())));
    }

    {
#if WIREFRAME_MODE_ENABLED
        rasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
#else
        rasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#endif
#if CULLING_ENABLED
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
#else
        rasterizerDesc.FrontCounterClockwise = false;
        rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
#endif
        rasterizerDesc.DepthClipEnable = true;
        rasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        rasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        rasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        rasterizerDesc.MultisampleEnable = FALSE;
        rasterizerDesc.AntialiasedLineEnable = FALSE;
        rasterizerDesc.ForcedSampleCount = 0;
        rasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    {
        blendDesc.AlphaToCoverageEnable = false;
        blendDesc.IndependentBlendEnable = false;
        blendDesc.RenderTarget[0].BlendEnable = false;
        blendDesc.RenderTarget[0].LogicOpEnable = false;
        blendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        blendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        blendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        blendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // NOTE(dima): create per-pass CB
    CreateBuffer(MAX_RENDER_GROUP_PER_PASS_CB_SIZE,
                 renderGroupPerPassCB.ReleaseAndGetAddressOf());

    // NOTE(dima): create per-object CB
    CreateBuffer(MAX_RENDER_GROUP_PER_OBJECT_CB_SIZE,
                 renderGroupPerObjectCB.ReleaseAndGetAddressOf());

    // Create PSO for rects
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

        desc.pRootSignature = rootSignature.Get();

        {
            debug_ReadFileResult vsFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_rect.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &vsFile, FILENAME);
#undef FILENAME
            ASSERT(vsFile.contentsSize);

            desc.VS = { vsFile.contents, vsFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &vsFile);
        }

        {
            debug_ReadFileResult psFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_rect.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &psFile, FILENAME);
#undef FILENAME
            ASSERT(psFile.contentsSize);

            desc.PS = { psFile.contents, psFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &psFile);
        }

        desc.RasterizerState = rasterizerDesc;
        desc.BlendState = blendDesc;

        {
            D3D12_DEPTH_STENCILOP_DESC DefaultStencilOp =  { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

            D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
            DepthStencilDesc.DepthEnable = true;  
            DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            DepthStencilDesc.StencilEnable = false;
            DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            DepthStencilDesc.FrontFace = DefaultStencilOp;
            DepthStencilDesc.BackFace = DefaultStencilOp;

            desc.DepthStencilState = DepthStencilDesc;
        }

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState[RECT_PIPELINE_STATE_IDX].ReleaseAndGetAddressOf())));
    }

    // Create PSO for lines
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

        {
            D3D12_INPUT_ELEMENT_DESC InputLayout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                                                       { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } };

            desc.InputLayout = { InputLayout, ARRAY_COUNT(InputLayout) };
        }

        desc.pRootSignature = rootSignature.Get();

        {
            debug_ReadFileResult vsFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_line.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &vsFile, FILENAME);
#undef FILENAME
            ASSERT(vsFile.contentsSize);

            desc.VS = { vsFile.contents, vsFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &vsFile);
        }

        {
            debug_ReadFileResult psFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_line.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &psFile, FILENAME);
#undef FILENAME
            ASSERT(psFile.contentsSize);

            desc.PS = { psFile.contents, psFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &psFile);
        }

        desc.RasterizerState = rasterizerDesc;
        desc.BlendState = blendDesc;

        {
            D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =  { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

            D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
            depthStencilDesc.DepthEnable = true;
            depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            depthStencilDesc.StencilEnable = false;
            depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            depthStencilDesc.FrontFace = defaultStencilOp;
            depthStencilDesc.BackFace = defaultStencilOp;

            desc.DepthStencilState = depthStencilDesc;
        }

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState[LINE_PIPELINE_STATE_IDX].ReleaseAndGetAddressOf())));
    }

    // Create PSO for points
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

        {
            D3D12_INPUT_ELEMENT_DESC InputLayout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                                                       { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } };

            desc.InputLayout = { InputLayout, ARRAY_COUNT(InputLayout) };
        }

        desc.pRootSignature = rootSignature.Get();

        {
            debug_ReadFileResult vsFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_point.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &vsFile, FILENAME);
#undef FILENAME
            ASSERT(vsFile.contentsSize);

            desc.VS = { vsFile.contents, vsFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &vsFile);
        }

        {
            debug_ReadFileResult psFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_point.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &psFile, FILENAME);
#undef FILENAME
            ASSERT(psFile.contentsSize);

            desc.PS = { psFile.contents, psFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &psFile);
        }

        desc.RasterizerState = rasterizerDesc;
        desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        desc.BlendState = blendDesc;

        {
            D3D12_DEPTH_STENCILOP_DESC DefaultStencilOp =  { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

            D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
            DepthStencilDesc.DepthEnable = true;
            DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            DepthStencilDesc.StencilEnable = false;
            DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            DepthStencilDesc.FrontFace = DefaultStencilOp;
            DepthStencilDesc.BackFace = DefaultStencilOp;

            desc.DepthStencilState = DepthStencilDesc;
        }

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState[POINT_PIPELINE_STATE_IDX].ReleaseAndGetAddressOf())));
    }

    // NOTE(dima): create PSO for tiles
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

        desc.pRootSignature = rootSignature.Get();

        {
            debug_ReadFileResult vsFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_tile.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &vsFile, FILENAME);
#undef FILENAME
            ASSERT(vsFile.contentsSize);

            desc.VS = { vsFile.contents, vsFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &vsFile);
        }

        {
            debug_ReadFileResult psFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_tile.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &psFile, FILENAME);
#undef FILENAME
            ASSERT(psFile.contentsSize);

            desc.PS = { psFile.contents, psFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &psFile);
        }

        desc.RasterizerState = rasterizerDesc;
        desc.BlendState = blendDesc;

        {
            D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =  { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

            D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
            depthStencilDesc.DepthEnable = true;  
            depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            depthStencilDesc.StencilEnable = false;
            depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            depthStencilDesc.FrontFace = defaultStencilOp;
            depthStencilDesc.BackFace = defaultStencilOp;

            desc.DepthStencilState = depthStencilDesc;
        }

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState[TILE_PIPELINE_STATE_IDX].ReleaseAndGetAddressOf())));
    }

    // NOTE(dima): create PSO for debugging textures
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC desc = {};

        desc.pRootSignature = rootSignature.Get();

        {
            debug_ReadFileResult vsFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_texture_debug.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &vsFile, FILENAME);
#undef FILENAME
            ASSERT(vsFile.contentsSize);

            desc.VS = { vsFile.contents, vsFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &vsFile);
        }

        {
            debug_ReadFileResult psFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_texture_debug.cso"
            gameMemory->debug_platformReadEntireFile(NULL, &psFile, FILENAME);
#undef FILENAME
            ASSERT(psFile.contentsSize);

            desc.PS = { psFile.contents, psFile.contentsSize };

            gameMemory->debug_platformFreeFileMemory(NULL, &psFile);
        }

        desc.RasterizerState = rasterizerDesc;
        desc.BlendState = blendDesc;

        {
            D3D12_DEPTH_STENCILOP_DESC defaultStencilOp =  { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

            D3D12_DEPTH_STENCIL_DESC depthStencilDesc = {};
            depthStencilDesc.DepthEnable = false;
            depthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            depthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            depthStencilDesc.StencilEnable = false;
            depthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            depthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            depthStencilDesc.FrontFace = defaultStencilOp;
            depthStencilDesc.BackFace = defaultStencilOp;

            desc.DepthStencilState = depthStencilDesc;
        }

        desc.SampleMask = UINT_MAX;
        desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        desc.NumRenderTargets = 1;
        desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        desc.SampleDesc.Count = 1;
        desc.SampleDesc.Quality = 0;
        desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        ThrowIfFailed(device->CreateGraphicsPipelineState(&desc, IID_PPV_ARGS(pipelineState[TEXTURE_DEBUG_PIPELINE_STATE_IDX].ReleaseAndGetAddressOf())));
    }
}

RENDER_ON_GPU(renderOnGPU)
{
    if (!win32State->windowVisible)
    {
        return;
    }

    GameMemory *gameMemory = (GameMemory *)win32State->gameMemory;

    // Populate command list
    {
        // Command list allocators can only be reset when the associated 
        // command lists have finished execution on the GPU; apps should use 
        // fences to determine GPU execution progress.
        ThrowIfFailed(commandAllocators[frameIndex]->Reset());

        // However, when ExecuteCommandList() is called on a particular command 
        // list, that command list can then be reset at any time and must be before 
        // re-recording.
        ThrowIfFailed(commandList->Reset(commandAllocators[frameIndex].Get(), NULL));

        // Indicate that the back buffer will be used as a render target.
        D3D12_RESOURCE_BARRIER Barrier = GetResourceTransitionBarrier(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
        commandList->ResourceBarrier(1, &Barrier);

        commandList->RSSetViewports(1, &viewport);
        commandList->RSSetScissorRects(1, &scissorRect);

        renderGroupVBCurrentSize = 0;
        currentCbvSrvUavDescriptorIdx = 0;
        currentPerObjectCBOffset = 0;

        D3D12_CPU_DESCRIPTOR_HANDLE rtvHandle = GetCPUDescriptorHandle(rtvHeap.Get(),
                                                                       rtvDescriptorSize,
                                                                       frameIndex);

        D3D12_CPU_DESCRIPTOR_HANDLE dsvHandle = dsvHeap->GetCPUDescriptorHandleForHeapStart();
        commandList->ClearDepthStencilView(dsvHandle,
                                           D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                           1.0f,
                                           0,
                                           0,
                                           NULL);

        commandList->OMSetRenderTargets(1, &rtvHandle, true, &dsvHandle);

#define CBV_SRV_UAV_HEAP_COUNT 1
        ID3D12DescriptorHeap** cbvSrvUavHeaps = PUSH_ARRAY(arena, CBV_SRV_UAV_HEAP_COUNT, ID3D12DescriptorHeap *);
        cbvSrvUavHeaps[0] = cbvSrvUavHeap[frameIndex].Get();
        commandList->SetDescriptorHeaps(CBV_SRV_UAV_HEAP_COUNT, cbvSrvUavHeaps);
#undef CBV_SRV_UAV_HEAP_COUNT

        commandList->SetGraphicsRootSignature(rootSignature.Get());

        D3D12_GPU_DESCRIPTOR_HANDLE perPassCBVGPUHndl = GetGPUDescriptorHandle(cbvSrvUavHeap[frameIndex].Get(),
                                                                               cbvSrvUavDescriptorSize,
                                                                               0);
                                                                               

        commandList->SetGraphicsRootDescriptorTable(0, perPassCBVGPUHndl);

        // NOTE(dima): populate per-pass CB
        {
            u8 *mappedData;
            ThrowIfFailed(renderGroupPerPassCB->Map(0, NULL, (void **)&mappedData));

            u32 dataLength = sizeof(renderGroup->uniforms);
            memcpy(mappedData, renderGroup->uniforms, dataLength);
            memcpy(mappedData + dataLength, &viewport.Width, sizeof(viewport.Width));
            memcpy(mappedData + dataLength + sizeof(viewport.Width), &viewport.Height, sizeof(viewport.Height));

            renderGroupPerPassCB->Unmap(0, NULL);

            u32 sizeOfData = RoundToNearestMultipleOf256(sizeof(renderGroup->uniforms) + sizeof(viewport.Width) + sizeof(viewport.Height));
            CreateConstantBufferView(renderGroupPerPassCB.Get(),
                                     cbvSrvUavHeap[frameIndex].Get(),
                                     cbvSrvUavDescriptorSize,
                                     currentCbvSrvUavDescriptorIdx,
                                     0,
                                     sizeOfData);
                                     
            ++currentCbvSrvUavDescriptorIdx;
        }

        // NOTE(dima): draw render items
        RenderGroupEntryHeader *entryHeader = (RenderGroupEntryHeader *)renderGroup->pushBufferBase;
        while (entryHeader)
        {
            ASSERT(renderGroupVBCurrentSize < MAX_RENDER_GROUP_VB_SIZE);
            ASSERT(currentPerObjectCBOffset < MAX_RENDER_GROUP_PER_OBJECT_CB_SIZE);

            switch (entryHeader->type)
            {
                case RenderGroupEntryType_RenderEntryClear:
                {
                    RenderEntryClear *entry = (RenderEntryClear *)(entryHeader + 1);

                    commandList->ClearRenderTargetView(rtvHandle, (r32 *)&entry->color, 0, NULL);

                    break;
                }

                case RenderGroupEntryType_RenderEntryPoint:
                {
                    commandList->SetPipelineState(pipelineState[POINT_PIPELINE_STATE_IDX].Get());

                    RenderEntryPoint *entry = (RenderEntryPoint *)(entryHeader + 1);

                    u32 renderGroupVBStartOffset = renderGroupVBCurrentSize;
                    {
                        r32 vertices[4 * 3 * 2];
                        V4 pointPosCamera = v4(entry->position.x,
                                               entry->position.y,
                                               entry->position.z,
                                               1.0f);
                        memcpy(vertices, (void*)&entry->position, sizeof(V3));
                        memcpy(vertices + 3, (void*)&entry->color, sizeof(entry->color));
                        memcpy(vertices + 6, (void*)&entry->position, sizeof(V3));
                        memcpy(vertices + 9, (void*)&entry->color, sizeof(entry->color));
                        memcpy(vertices + 12, (void*)&entry->position, sizeof(V3));
                        memcpy(vertices + 15, (void*)&entry->color, sizeof(entry->color));
                        memcpy(vertices + 18, (void*)&entry->position, sizeof(V3));
                        memcpy(vertices + 21, (void*)&entry->color, sizeof(entry->color));

                        u32 sizeOfData = sizeof(r32)*ARRAY_COUNT(vertices);
                        renderGroupVBCurrentSize += sizeOfData;

                        u8 *mappedData;
                        ThrowIfFailed(renderGroupVb->Map(0, NULL, (void **)&mappedData));
                        mappedData += renderGroupVBStartOffset;

                        memcpy(mappedData, vertices, sizeOfData);

                        renderGroupVb->Unmap(0, NULL);

                        D3D12_VERTEX_BUFFER_VIEW view;
                        view.BufferLocation = renderGroupVb->GetGPUVirtualAddress() + renderGroupVBStartOffset;
                        view.SizeInBytes = renderGroupVBCurrentSize - renderGroupVBStartOffset;
                        // TODO(dima): currently, will have to modify this every time InputLayout changes
                        view.StrideInBytes = 2*sizeof(V3);

                        commandList->IASetVertexBuffers(0, 1, &view);
                    }

                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                    commandList->DrawInstanced(4, 1, 0, 0);

                    break;
                }

                case RenderGroupEntryType_RenderEntryLine:
                {
                    commandList->SetPipelineState(pipelineState[LINE_PIPELINE_STATE_IDX].Get());

                    RenderEntryLine *entry = (RenderEntryLine *)(entryHeader + 1);

                    u32 renderGroupVBStartOffset = renderGroupVBCurrentSize;
                    {
                        r32 vertices[2 * 3 * 2];
                        memcpy(vertices, (void*)&entry->start, sizeof(entry->start));
                        memcpy(vertices + 3, (void*)&entry->color, sizeof(entry->color));
                        memcpy(vertices + 6, (void*)&entry->end, sizeof(entry->end));
                        memcpy(vertices + 9, (void*)&entry->color, sizeof(entry->color));

                        u32 sizeOfData = sizeof(r32)*ARRAY_COUNT(vertices);
                        renderGroupVBCurrentSize += sizeOfData;

                        u8 *mappedData;
                        ThrowIfFailed(renderGroupVb->Map(0, NULL, (void **)&mappedData));
                        mappedData += renderGroupVBStartOffset;

                        memcpy(mappedData, vertices, sizeOfData);

                        renderGroupVb->Unmap(0, NULL);

                        D3D12_VERTEX_BUFFER_VIEW view;
                        view.BufferLocation = renderGroupVb->GetGPUVirtualAddress() + renderGroupVBStartOffset;
                        view.SizeInBytes = renderGroupVBCurrentSize - renderGroupVBStartOffset;
                        // TODO(dima): currently, will have to modify this every time InputLayout changes
                        view.StrideInBytes = 2*sizeof(V3);

                        commandList->IASetVertexBuffers(0, 1, &view);
                    }

                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

                    commandList->DrawInstanced(2, 1, 0, 0);

                    break;
                }

                case RenderGroupEntryType_RenderEntryTile:
                {
                    commandList->SetPipelineState(pipelineState[TILE_PIPELINE_STATE_IDX].Get());

                    RenderEntryTile *entry = (RenderEntryTile *)(entryHeader + 1);

                    D3D12_GPU_VIRTUAL_ADDRESS cbAddress = renderGroupPerObjectCB->GetGPUVirtualAddress() 
                                                          + currentPerObjectCBOffset;
                    commandList->SetGraphicsRootConstantBufferView(2, cbAddress);

                    {
                        u32 lengthOfTileCountPerSide = sizeof(entry->tileCountPerSide);
                        u32 lengthOfTileSideLength = sizeof(entry->tileSideLength);
                        u32 lengthOfTileColor = sizeof(entry->color);
                        u32 lengthOfOriginTileCenterPositionWorld = sizeof(entry->originTileCenterPositionWorld);
                        u32 totalSize = RoundToNearestMultipleOf256(lengthOfTileCountPerSide
                                                                    + lengthOfTileSideLength
                                                                    + lengthOfTileColor
                                                                    + lengthOfOriginTileCenterPositionWorld);
                        u8 *mappedData;
                        ThrowIfFailed(renderGroupPerObjectCB->Map(0, NULL, (void **)&mappedData));

                        mappedData += currentPerObjectCBOffset;
                        memcpy(mappedData, &entry->color, lengthOfTileColor);
                        mappedData += lengthOfTileColor
                                      // NOTE(dima): padding to respect HLSL constant buffer's memory alignment rules
                                      + sizeof(r32);
                        memcpy(mappedData, &entry->originTileCenterPositionWorld, lengthOfOriginTileCenterPositionWorld);
                        mappedData += lengthOfOriginTileCenterPositionWorld;
                        memcpy(mappedData, &entry->tileCountPerSide, lengthOfTileCountPerSide);
                        mappedData += lengthOfTileCountPerSide;
                        memcpy(mappedData, &entry->tileSideLength, lengthOfTileSideLength);

                        renderGroupPerObjectCB->Unmap(0, NULL);

                        // NOTE(dima): create CBV for the data we just copied
                        CreateConstantBufferView(renderGroupPerObjectCB.Get(),
                                                 cbvSrvUavHeap[frameIndex].Get(),
                                                 cbvSrvUavDescriptorSize,
                                                 currentCbvSrvUavDescriptorIdx,
                                                 currentPerObjectCBOffset,
                                                 totalSize);
                                                 
                        ++currentCbvSrvUavDescriptorIdx;

                        currentPerObjectCBOffset += totalSize;
                    }

                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                    commandList->DrawInstanced(4, entry->tileCountPerSide * entry->tileCountPerSide, 0, 0);

                } break;

                case RenderGroupEntryType_RenderEntryRect:
                {
                    commandList->SetPipelineState(pipelineState[RECT_PIPELINE_STATE_IDX].Get());

                    RenderEntryRect *entry = (RenderEntryRect *)(entryHeader + 1);

                    D3D12_GPU_DESCRIPTOR_HANDLE perObjectCbvGpuHndl = GetGPUDescriptorHandle(cbvSrvUavHeap[frameIndex].Get(),
                                                                                             cbvSrvUavDescriptorSize,
                                                                                             currentCbvSrvUavDescriptorIdx);
                                                                                             
                    commandList->SetGraphicsRootDescriptorTable(1, perObjectCbvGpuHndl);

                    {
                        u32 sideLengthWLength = sizeof(entry->sideLengthW);
                        u32 sideLengthHLength = sizeof(entry->sideLengthH);
                        u32 rectCenterPositionWorldLength = sizeof(entry->rectCenterPositionWorld);
                        u32 colorLength = sizeof(entry->color);
                        u32 totalSize = RoundToNearestMultipleOf256(sideLengthWLength
                                                                    + sideLengthHLength
                                                                    + rectCenterPositionWorldLength
                                                                    + colorLength);

                        u8 *mappedData;
                        ThrowIfFailed(renderGroupPerObjectCB->Map(0, NULL, (void **)&mappedData));

                        mappedData += currentPerObjectCBOffset;

                        memcpy(mappedData, &entry->rectCenterPositionWorld, rectCenterPositionWorldLength);
                        mappedData += rectCenterPositionWorldLength;

                        memcpy(mappedData, &entry->sideLengthW, sideLengthWLength);
                        mappedData += sideLengthWLength;

                        memcpy(mappedData, &entry->color, colorLength);
                        mappedData += colorLength;

                        memcpy(mappedData, &entry->sideLengthH, sideLengthHLength);

                        renderGroupPerObjectCB->Unmap(0, NULL);

                        // NOTE(dima): create CBV for the data we just copied
                        CreateConstantBufferView(renderGroupPerObjectCB.Get(),
                                                 cbvSrvUavHeap[frameIndex].Get(),
                                                 cbvSrvUavDescriptorSize,
                                                 currentCbvSrvUavDescriptorIdx,
                                                 currentPerObjectCBOffset,
                                                 totalSize);
                                                 
                        ++currentCbvSrvUavDescriptorIdx;

                        currentPerObjectCBOffset += totalSize;
                    }

                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                    commandList->DrawInstanced(4, 1, 0, 0);

                    break;
                }

#if 1
                case RenderGroupEntryType_RenderEntryTextureDebug:
                {
                    RenderEntryTextureDebug *entry = (RenderEntryTextureDebug *)(entryHeader + 1);
                    AssetHeader *textureHeader = entry->textureHeader;

                    commandList->SetPipelineState(pipelineState[TEXTURE_DEBUG_PIPELINE_STATE_IDX].Get());

                    D3D12_GPU_DESCRIPTOR_HANDLE srvGpuHndl = GetGPUDescriptorHandle(cbvSrvUavHeap[frameIndex].Get(),
                                                                                    cbvSrvUavDescriptorSize,
                                                                                    currentCbvSrvUavDescriptorIdx);
                                                                                    
                    commandList->SetGraphicsRootDescriptorTable(3, srvGpuHndl);

                    DXGI_FORMAT textureFormat = DXGI_FORMAT_R8G8B8A8_UNORM;

                    if (!texture[frameIndex].initialized)
                    {
                        D3D12_RESOURCE_DESC TextureDesc = GetResourceDescriptor(D3D12_RESOURCE_DIMENSION_TEXTURE2D,
                                                                                0,
                                                                                textureHeader->width,
                                                                                textureHeader->height,
                                                                                1,
                                                                                1,
                                                                                textureFormat,
                                                                                1,
                                                                                0,
                                                                                D3D12_TEXTURE_LAYOUT_UNKNOWN,
                                                                                D3D12_RESOURCE_FLAG_NONE);

                        D3D12_HEAP_PROPERTIES textureHeapProperties = GetHeapProperties(D3D12_HEAP_TYPE_DEFAULT);
                        ThrowIfFailed(device->CreateCommittedResource(&textureHeapProperties,
                                                                      D3D12_HEAP_FLAG_NONE,
                                                                      &TextureDesc,
                                                                      D3D12_RESOURCE_STATE_COPY_DEST,
                                                                      NULL,
                                                                      IID_PPV_ARGS(texture[frameIndex].gpuTexture.ReleaseAndGetAddressOf())));

                        texture[frameIndex].gpuTexture->SetName(L"Texture");

                        texture[frameIndex].state = D3D12_RESOURCE_STATE_COPY_DEST;
                        texture[frameIndex].needsGpuReupload = true;
                        texture[frameIndex].initialized = true;
                    }

                    u32 textureDataSize = textureHeader->width*textureHeader->height*textureHeader->pixelSizeBytes;

                    // TODO: upload heap should be large enough to fit any texture!
                    if (!textureUploadHeap[frameIndex])
                    {
                        // Create the GPU upload buffer.
                        D3D12_HEAP_PROPERTIES bufferResourceHeapProperties = GetHeapProperties(D3D12_HEAP_TYPE_UPLOAD);

                        D3D12_RESOURCE_DESC bufferResourceDesc = GetBufferResourceDescriptor(textureDataSize);
                        ThrowIfFailed(device->CreateCommittedResource(&bufferResourceHeapProperties,
                                                                      D3D12_HEAP_FLAG_NONE,
                                                                      &bufferResourceDesc,
                                                                      D3D12_RESOURCE_STATE_GENERIC_READ,
                                                                      NULL,
                                                                      IID_PPV_ARGS(textureUploadHeap[frameIndex].ReleaseAndGetAddressOf())));

                        textureUploadHeap[frameIndex]->SetName(L"textureUploadHeap");
                    }

                    if (entry->needsGpuReupload)
                    {
                        for (u32 frameIdx = 0; frameIdx < SWAP_CHAIN_BUFFER_COUNT; ++frameIdx)
                        {
                            texture[frameIdx].needsGpuReupload = true;
                        }
                    }

                    if (texture[frameIndex].needsGpuReupload)
                    {
                        D3D12_RESOURCE_BARRIER textureBarrier;
                        if (texture[frameIndex].state == D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE)
                        {
                            textureBarrier = GetResourceTransitionBarrier(texture[frameIndex].gpuTexture.Get(), D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE, D3D12_RESOURCE_STATE_COPY_DEST);
                            commandList->ResourceBarrier(1, &textureBarrier);
                        }

                        u8 *mappedData;
                        ThrowIfFailed(textureUploadHeap[frameIndex]->Map(0, NULL, (void **)&mappedData));

                        u8 *rgbaBitmap = ((u8 *)textureHeader) + sizeof(AssetHeader);
                        memcpy(mappedData, rgbaBitmap, textureDataSize);

                        textureUploadHeap[frameIndex]->Unmap(0, NULL);

                        D3D12_SUBRESOURCE_FOOTPRINT srcFootprint = {};
                        srcFootprint.Format = textureFormat;
                        srcFootprint.Width = textureHeader->width;
                        srcFootprint.Height = textureHeader->height;
                        srcFootprint.Depth = 1;
                        srcFootprint.RowPitch = textureHeader->pixelSizeBytes*textureHeader->width;

                        D3D12_PLACED_SUBRESOURCE_FOOTPRINT srcPlacedFootprint = {};
                        srcPlacedFootprint.Offset = 0;
                        srcPlacedFootprint.Footprint = srcFootprint;

                        D3D12_TEXTURE_COPY_LOCATION src = {};
                        src.pResource = textureUploadHeap[frameIndex].Get();
                        src.Type = D3D12_TEXTURE_COPY_TYPE_PLACED_FOOTPRINT;
                        src.PlacedFootprint = srcPlacedFootprint;

                        D3D12_TEXTURE_COPY_LOCATION dst = {};
                        dst.pResource = texture[frameIndex].gpuTexture.Get();
                        dst.Type = D3D12_TEXTURE_COPY_TYPE_SUBRESOURCE_INDEX;
                        dst.SubresourceIndex = 0;

                        commandList->CopyTextureRegion(&dst, 0, 0, 0, &src, NULL);

                        textureBarrier = GetResourceTransitionBarrier(texture[frameIndex].gpuTexture.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE);
                        commandList->ResourceBarrier(1, &textureBarrier);
                        texture[frameIndex].state = D3D12_RESOURCE_STATE_PIXEL_SHADER_RESOURCE;

                        texture[frameIndex].needsGpuReupload = false;
                    }

                    D3D12_TEX2D_SRV TexSRV =  {};
                    TexSRV.MostDetailedMip = 0;
                    TexSRV.MipLevels = 1;
                    TexSRV.ResourceMinLODClamp = 0.0f;

                    D3D12_SHADER_RESOURCE_VIEW_DESC Desc = {};
                    Desc.Format = textureFormat;
                    Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
                    Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
                    Desc.Texture2D = TexSRV;

                    D3D12_CPU_DESCRIPTOR_HANDLE srvCpuHndl = GetCPUDescriptorHandle(cbvSrvUavHeap[frameIndex].Get(), cbvSrvUavDescriptorSize, currentCbvSrvUavDescriptorIdx);

                    device->CreateShaderResourceView(texture[frameIndex].gpuTexture.Get(),
                                                     &Desc,
                                                     srvCpuHndl);


                    ++currentCbvSrvUavDescriptorIdx;

                    commandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                    commandList->DrawInstanced(4, 1, 0, 0);

                    break;
                }
#endif

                default:
                    break;
            }

            entryHeader = entryHeader->next;
        }

        // Indicate that the back buffer will now be used to present.
        Barrier = GetResourceTransitionBarrier(renderTargets[frameIndex].Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
        commandList->ResourceBarrier(1, &Barrier);

        ThrowIfFailed(commandList->Close());
    }

    // Execute the command list.
#define COMMAND_LIST_COUNT 1
    ID3D12CommandList** ppCommandLists = PUSH_ARRAY(arena, COMMAND_LIST_COUNT, ID3D12CommandList *);
    ppCommandLists[0] = commandList.Get();
    commandQueue->ExecuteCommandLists(COMMAND_LIST_COUNT, ppCommandLists);
#undef COMMAND_LIST_COUNT

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

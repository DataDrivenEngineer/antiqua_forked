#include <d3d12.h>
#include <dxgi1_4.h>
#include <dxgi1_5.h>
#include <dxgidebug.h>
#include <wrl.h>

#include "win32_antiqua.h"
#include "antiqua.h"
#include "antiqua_render_group.h"

#define CULLING_ENABLED 1
#define WIREFRAME_MODE_ENABLED 0

internal Microsoft::WRL::ComPtr<ID3D12Device> Device;

internal Microsoft::WRL::ComPtr<ID3D12Fence> Fence;
internal u64 CurrentFence = 0;

internal u32 RTVDescriptorSize;
internal u32 DSVDescriptorSize;
internal u32 CBV_SRV_UAV_DescriptorSize;

internal Microsoft::WRL::ComPtr<ID3D12Resource> depthStencilBuffer = NULL;

internal Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> RTVHeap;
internal Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> DSVHeap;
internal Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> CBV_SRV_UAV_Heap;

internal Microsoft::WRL::ComPtr<ID3D12CommandQueue> CmdQueue;
internal Microsoft::WRL::ComPtr<ID3D12CommandAllocator> CmdAlloc;
internal Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> CmdList;

internal Microsoft::WRL::ComPtr<IDXGISwapChain> SwapChain;
#define SWAP_CHAIN_BUFFER_COUNT 2
internal u32 CurrentBackBufferIdx = 0;
internal Microsoft::WRL::ComPtr<ID3D12Resource> SwapChainBuffer[] = {NULL, NULL};

internal Microsoft::WRL::ComPtr<ID3D12RootSignature> RootSignature;
#define RECT_PIPELINE_STATE_IDX 0
#define LINE_PIPELINE_STATE_IDX 1
#define POINT_PIPELINE_STATE_IDX 2
#define TILE_PIPELINE_STATE_IDX 3
#define TEXTURE_DEBUG_PIPELINE_STATE_IDX 4

#define PIPELINE_STATE_COUNT 5
internal Microsoft::WRL::ComPtr<ID3D12PipelineState> PipelineState[PIPELINE_STATE_COUNT];

#define MAX_RENDER_GROUP_VB_SIZE KB(256)
internal Microsoft::WRL::ComPtr<ID3D12Resource> RenderGroupVB;
internal u32 renderGroupVBCurrentSize = 0;

// NOTE(dima): CB must be of size which is multiple of 256 bytes
#define MAX_RENDER_GROUP_PER_OBJECT_CB_SIZE KB(512)
internal Microsoft::WRL::ComPtr<ID3D12Resource> RenderGroupPerObjectCB;
internal u32 renderGroupCBCurrentSize = 0;
internal u32 Current_CBV_SRV_UAV_DescriptorIdx = 0;
internal u32 CurrentPerObjectCBOffset = 0;

#define MAX_RENDER_GROUP_PER_PASS_CB_SIZE KB(1)
internal Microsoft::WRL::ComPtr<ID3D12Resource> RenderGroupPerPassCB;

internal D3D12_VIEWPORT Viewport = {};

internal D3D12_RECT ScissorRect;

internal D3D12_RASTERIZER_DESC RasterizerDesc = {};
internal D3D12_BLEND_DESC BlendDesc = {};

internal Microsoft::WRL::ComPtr<ID3D12Resource> Texture;

void ReportLiveObjects()
{
    Microsoft::WRL::ComPtr<IDXGIDebug1> dxgi_debug;
    if (SUCCEEDED(DXGIGetDebugInterface1(0, IID_PPV_ARGS(dxgi_debug.GetAddressOf()))))
    {
            dxgi_debug->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS(DXGI_DEBUG_RLO_DETAIL | DXGI_DEBUG_RLO_IGNORE_INTERNAL));
    }
}

internal inline b32 TearingSupportEnabled()
{
#if 0
    IDXGIFactory4 *Factory4;
    HRESULT Hr = CreateDXGIFactory1(IID_PPV_ARGS(&Factory4));
    b32 AllowTearing = false;
    if (SUCCEEDED(Hr))
    { 
        IDXGIFactory5 *Factory5 = (IDXGIFactory5 *)Factory4;
        Hr = Factory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &AllowTearing, sizeof(AllowTearing));
    }
    AllowTearing = SUCCEEDED(Hr) && AllowTearing;
    return AllowTearing;
#else
    return false;
#endif
}

internal inline D3D12_GPU_DESCRIPTOR_HANDLE
GetGPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap, u32 DescriptorSize, u32 DescriptorIdx)
{
    D3D12_GPU_DESCRIPTOR_HANDLE StartHndl = Heap->GetGPUDescriptorHandleForHeapStart();
    D3D12_GPU_DESCRIPTOR_HANDLE CurrHndl;
    CurrHndl.ptr = StartHndl.ptr + DescriptorIdx*DescriptorSize;

    return CurrHndl;
}

internal inline D3D12_CPU_DESCRIPTOR_HANDLE
GetCPUDescriptorHandle(Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap, u32 DescriptorSize, u32 DescriptorIdx)
{
    D3D12_CPU_DESCRIPTOR_HANDLE StartHndl = Heap->GetCPUDescriptorHandleForHeapStart();
    D3D12_CPU_DESCRIPTOR_HANDLE CurrHndl;
    CurrHndl.ptr = StartHndl.ptr + DescriptorIdx*DescriptorSize;

    return CurrHndl;
}

internal inline void CreateConstantBufferView(Microsoft::WRL::ComPtr<ID3D12Resource> Buffer,
                                              Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> Heap,
                                              u32 CBV_SRV_UAV_DescSize,
                                              u32 CBV_SRV_UAV_DescriptorIdx,
                                              u32 CurrentCBOffset,
                                              u32 SizeOfDataRoundedToNearest256)
{
    D3D12_GPU_VIRTUAL_ADDRESS CBAddress = Buffer->GetGPUVirtualAddress() + CurrentCBOffset;
    D3D12_CPU_DESCRIPTOR_HANDLE CurrHndl = GetCPUDescriptorHandle(Heap,
                                                                  CBV_SRV_UAV_DescSize,
                                                                  CBV_SRV_UAV_DescriptorIdx);

    D3D12_CONSTANT_BUFFER_VIEW_DESC Desc;
    Desc.BufferLocation = CBAddress;
    Desc.SizeInBytes = SizeOfDataRoundedToNearest256;

    Device->CreateConstantBufferView(&Desc, CurrHndl);
}

internal inline void FlushCommandQueue()
{
    ++CurrentFence;

    CmdQueue->Signal(Fence.Get(), CurrentFence);

    if (Fence->GetCompletedValue() < CurrentFence)
    {
        HANDLE EventHandle = CreateEventEx(NULL, false, false, EVENT_ALL_ACCESS);

        Fence->SetEventOnCompletion(CurrentFence, EventHandle);

        WaitForSingleObject(EventHandle, INFINITE);

        CloseHandle(EventHandle);
    }
}

internal inline void CreateRenderTargetViews()
{
    for (u32 i = 0;
         i < SWAP_CHAIN_BUFFER_COUNT;
         ++i)
    {
        HRESULT Hr = SwapChain->GetBuffer(i, IID_PPV_ARGS(SwapChainBuffer[i].GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));

        D3D12_CPU_DESCRIPTOR_HANDLE CurrHndl = GetCPUDescriptorHandle(RTVHeap,
                                                                      RTVDescriptorSize,
                                                                      i);

        Device->CreateRenderTargetView(SwapChainBuffer[i].Get(), 0, CurrHndl);
    }
}

internal inline void CreateDepthStencilBuffer(s32 WindowWidth, s32 WindowHeight)
{
    depthStencilBuffer.Reset();

    D3D12_RESOURCE_DESC	DepthStencilDesc = {};
    DepthStencilDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
    DepthStencilDesc.Alignment = 0;
    DepthStencilDesc.Width = WindowWidth;
    DepthStencilDesc.Height = WindowHeight;
    DepthStencilDesc.DepthOrArraySize = 1;
    DepthStencilDesc.MipLevels = 1;
    DepthStencilDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    DepthStencilDesc.SampleDesc.Count = 1;
    DepthStencilDesc.SampleDesc.Quality = 0;
    DepthStencilDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
    DepthStencilDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

    D3D12_CLEAR_VALUE Clear = {};
    Clear.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
    Clear.DepthStencil.Depth = 1.0f;
    Clear.DepthStencil.Stencil = 0;

    D3D12_HEAP_PROPERTIES HeapProperties = {};
    HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
    HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
    HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
    HeapProperties.CreationNodeMask = 1;
    HeapProperties.VisibleNodeMask = 1;

    HRESULT Hr = Device->CreateCommittedResource(&HeapProperties,
                                                 D3D12_HEAP_FLAG_NONE,
                                                 &DepthStencilDesc,
                                                 D3D12_RESOURCE_STATE_COMMON,
                                                 &Clear,
                                                 IID_PPV_ARGS(depthStencilBuffer.GetAddressOf()));
    ASSERT(SUCCEEDED(Hr));

    D3D12_DEPTH_STENCIL_VIEW_DESC DSVDesc = {};
    DSVDesc.Flags = D3D12_DSV_FLAG_NONE;
	DSVDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	DSVDesc.Format = DXGI_FORMAT_D24_UNORM_S8_UINT;
	DSVDesc.Texture2D.MipSlice = 0;

    Device->CreateDepthStencilView(depthStencilBuffer.Get(),
                                   &DSVDesc,
                                   DSVHeap->GetCPUDescriptorHandleForHeapStart());

    D3D12_RESOURCE_TRANSITION_BARRIER TransitionBarrier = {};
    TransitionBarrier.pResource = depthStencilBuffer.Get();
    TransitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
    TransitionBarrier.StateBefore = D3D12_RESOURCE_STATE_COMMON;
    TransitionBarrier.StateAfter = D3D12_RESOURCE_STATE_DEPTH_WRITE;

    D3D12_RESOURCE_BARRIER Barrier = {};
    Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
    Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
    Barrier.Transition = TransitionBarrier;

    CmdList->ResourceBarrier(1, &Barrier);
}

RESIZE_WINDOW(ResizeWindow)
{
    HWND Window = *((HWND *)data);
    RECT Rect;
    if (GetWindowRect(Window, &Rect))
    {
        s32 NewWidth = Rect.right - Rect.left;
        s32 NewHeight = Rect.bottom - Rect.top;

        FlushCommandQueue();

        CmdList->Reset(CmdAlloc.Get(), NULL);

        // NOTE(dima): resize swap chain buffers
        {
// NOTE(dima): removing due to warnings in output
#if 1
            for (u32 swapChainBufIdx = 0;
                 swapChainBufIdx < SWAP_CHAIN_BUFFER_COUNT;
                 ++swapChainBufIdx)
            {
                if (SwapChainBuffer[swapChainBufIdx])
                {
                    SwapChainBuffer[swapChainBufIdx].Reset();
                }
            }
#endif

            HRESULT Hr = SwapChain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT,
                                                  NewWidth,
                                                  NewHeight,
                                                  DXGI_FORMAT_R8G8B8A8_UNORM,
                                                  DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH);
            ASSERT(SUCCEEDED(Hr));

            CurrentBackBufferIdx = 0;

            CreateRenderTargetViews();
        }

        CreateDepthStencilBuffer(NewWidth, NewHeight);

        CmdList->Close();
        CmdQueue->ExecuteCommandLists(1, (ID3D12CommandList **)CmdList.GetAddressOf());

        FlushCommandQueue();

        Viewport.TopLeftX = 0.0f;
        Viewport.TopLeftY = 0.0f;
        Viewport.Width = (r32)NewWidth;
        Viewport.Height = (r32)NewHeight;
        Viewport.MinDepth = 0.0f;
        Viewport.MaxDepth = 1.0f;

        ScissorRect = {0, 0, NewWidth, NewHeight};
    }
}

INIT_RENDERER(initRenderer)
{
    HWND Window = ((InitRendererData *)data)->Window;
    u32 WindowWidth = ((InitRendererData *)data)->WindowWidth;
    u32 WindowHeight = ((InitRendererData *)data)->WindowHeight;
    GameMemory *Memory = ((InitRendererData *)data)->Memory;

    HRESULT Hr;
    
#if ANTIQUA_SLOW
    Microsoft::WRL::ComPtr<ID3D12Debug> DebugController;
    Hr = D3D12GetDebugInterface(IID_PPV_ARGS(DebugController.GetAddressOf()));
    ASSERT(SUCCEEDED(Hr));
    DebugController->EnableDebugLayer();
#endif

    Hr = D3D12CreateDevice(0, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(Device.GetAddressOf()));
    ASSERT(SUCCEEDED(Hr));

    Hr = Device->CreateFence(0,	D3D12_FENCE_FLAG_NONE,IID_PPV_ARGS(Fence.GetAddressOf()));
    ASSERT(SUCCEEDED(Hr));

    // NOTE(dima): Get descriptor sizes and create descriptor heaps
    {
        RTVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
        DSVDescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
        CBV_SRV_UAV_DescriptorSize = Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

        D3D12_DESCRIPTOR_HEAP_DESC RTVDesc = {};
        RTVDesc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
        RTVDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
        RTVDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        RTVDesc.NodeMask = 0;

        Hr = Device->CreateDescriptorHeap(&RTVDesc, IID_PPV_ARGS(RTVHeap.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));

        D3D12_DESCRIPTOR_HEAP_DESC DSVDesc = {};
        DSVDesc.NumDescriptors = 1;
        DSVDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
        DSVDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
        DSVDesc.NodeMask = 0;

        Hr = Device->CreateDescriptorHeap(&DSVDesc, IID_PPV_ARGS(DSVHeap.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    {
        D3D12_COMMAND_QUEUE_DESC Desc = {};
        Desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
        Desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
        Hr = Device->CreateCommandQueue(&Desc, IID_PPV_ARGS(CmdQueue.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    Hr = Device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(CmdAlloc.GetAddressOf()));
    ASSERT(SUCCEEDED(Hr));

    Hr = Device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, CmdAlloc.Get(), 0, IID_PPV_ARGS(CmdList.GetAddressOf()));
    ASSERT(SUCCEEDED(Hr));
    CmdList->Close();

    // NOTE(dima): create swap chain
    {
        SwapChain.Reset();

        Microsoft::WRL::ComPtr<IDXGIFactory4> DXGIFactory;
        CreateDXGIFactory1(IID_PPV_ARGS(DXGIFactory.GetAddressOf()));

        DXGI_SWAP_CHAIN_DESC Desc = {};
        Desc.BufferDesc.Width = 0;
        Desc.BufferDesc.Height = 0;

        // TODO(dima): determine these programmatically from monitor info
        Desc.BufferDesc.RefreshRate.Numerator = 60;
        Desc.BufferDesc.RefreshRate.Denominator = 1;

        Desc.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
        Desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
        // TODO(dima): Do we need to determine it programmatically?
        Desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
        Desc.OutputWindow = Window;
        Desc.Windowed = true;
        Desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
        Desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

        Hr = DXGIFactory->CreateSwapChain(CmdQueue.Get(), &Desc, SwapChain.GetAddressOf());
        ASSERT(SUCCEEDED(Hr));
    }

    ResizeWindow(&Window);

    // NOTE(dima): create VB
    {
        D3D12_RESOURCE_DESC	Desc = {};
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Alignment = 0;
        Desc.Width = MAX_RENDER_GROUP_VB_SIZE;
        Desc.Height = 1;
        Desc.DepthOrArraySize = 1;
        Desc.MipLevels = 1;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProperties = {};
        HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.CreationNodeMask = 1;
        HeapProperties.VisibleNodeMask = 1;

        Hr = Device->CreateCommittedResource(&HeapProperties,
                                             D3D12_HEAP_FLAG_NONE,
                                             &Desc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                             NULL,
                                             IID_PPV_ARGS(RenderGroupVB.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // NOTE(dima): create CB Descriptor Heap
    {
        // NOTE(dima): descriptor per entity + 1 for per pass descirptor + 1 for SRV Descriptor
        // TODO(dima): do not hardcode max entity count!
        u32 DescriptorCount = (256 + 1 + 1);

        D3D12_DESCRIPTOR_HEAP_DESC Desc = {};
        Desc.NumDescriptors = DescriptorCount;
        Desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
        Desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
        Desc.NodeMask = 0;

        Hr = Device->CreateDescriptorHeap(&Desc, IID_PPV_ARGS(CBV_SRV_UAV_Heap.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // NOTE(dima): create per-object CB
    {
        D3D12_RESOURCE_DESC	Desc = {};
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Alignment = 0;
        Desc.Width = MAX_RENDER_GROUP_PER_OBJECT_CB_SIZE;
        Desc.Height = 1;
        Desc.DepthOrArraySize = 1;
        Desc.MipLevels = 1;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProperties = {};
        HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.CreationNodeMask = 1;
        HeapProperties.VisibleNodeMask = 1;

        Hr = Device->CreateCommittedResource(&HeapProperties,
                                             D3D12_HEAP_FLAG_NONE,
                                             &Desc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                             NULL,
                                             IID_PPV_ARGS(RenderGroupPerObjectCB.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // NOTE(dima): create per-pass CB
    {
        D3D12_RESOURCE_DESC	Desc = {};
        Desc.Dimension = D3D12_RESOURCE_DIMENSION_BUFFER;
        Desc.Alignment = 0;
        Desc.Width = MAX_RENDER_GROUP_PER_PASS_CB_SIZE;
        Desc.Height = 1;
        Desc.DepthOrArraySize = 1;
        Desc.MipLevels = 1;
        Desc.Format = DXGI_FORMAT_UNKNOWN;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR;
        Desc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProperties = {};
        HeapProperties.Type = D3D12_HEAP_TYPE_UPLOAD;
        HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.CreationNodeMask = 1;
        HeapProperties.VisibleNodeMask = 1;

        Hr = Device->CreateCommittedResource(&HeapProperties,
                                             D3D12_HEAP_FLAG_NONE,
                                             &Desc,
                                             D3D12_RESOURCE_STATE_GENERIC_READ,
                                             NULL,
                                             IID_PPV_ARGS(RenderGroupPerPassCB.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    {
#if WIREFRAME_MODE_ENABLED
        RasterizerDesc.FillMode = D3D12_FILL_MODE_WIREFRAME;
#else
        RasterizerDesc.FillMode = D3D12_FILL_MODE_SOLID;
#endif
#if CULLING_ENABLED
        RasterizerDesc.FrontCounterClockwise = false;
        RasterizerDesc.CullMode = D3D12_CULL_MODE_BACK;
#else
        RasterizerDesc.FrontCounterClockwise = false;
        RasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
#endif
        RasterizerDesc.DepthClipEnable = true;
        RasterizerDesc.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
        RasterizerDesc.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
        RasterizerDesc.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
        RasterizerDesc.MultisampleEnable = FALSE;
        RasterizerDesc.AntialiasedLineEnable = FALSE;
        RasterizerDesc.ForcedSampleCount = 0;
        RasterizerDesc.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
    }

    {
        BlendDesc.AlphaToCoverageEnable = false;
        BlendDesc.IndependentBlendEnable = false;
        BlendDesc.RenderTarget[0].BlendEnable = false;
        BlendDesc.RenderTarget[0].LogicOpEnable = false;
        BlendDesc.RenderTarget[0].SrcBlend = D3D12_BLEND_ONE;
        BlendDesc.RenderTarget[0].DestBlend = D3D12_BLEND_ZERO;
        BlendDesc.RenderTarget[0].BlendOp = D3D12_BLEND_OP_ADD;
        BlendDesc.RenderTarget[0].SrcBlendAlpha = D3D12_BLEND_ONE;
        BlendDesc.RenderTarget[0].DestBlendAlpha = D3D12_BLEND_ZERO;
        BlendDesc.RenderTarget[0].BlendOpAlpha = D3D12_BLEND_OP_ADD;
        BlendDesc.RenderTarget[0].LogicOp = D3D12_LOGIC_OP_NOOP;
        BlendDesc.RenderTarget[0].RenderTargetWriteMask = D3D12_COLOR_WRITE_ENABLE_ALL;
    }

    // NOTE(dima): create root signature
    {
        D3D12_DESCRIPTOR_RANGE PerPassRange = {};
        PerPassRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        PerPassRange.NumDescriptors = 1;
        PerPassRange.BaseShaderRegister = 0;
        PerPassRange.RegisterSpace = 0;
        PerPassRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_DESCRIPTOR_RANGE PerObjectRange = {};
        PerObjectRange.RangeType = D3D12_DESCRIPTOR_RANGE_TYPE_CBV;
        PerObjectRange.NumDescriptors = 1;
        PerObjectRange.BaseShaderRegister = 1;
        PerObjectRange.RegisterSpace = 0;
        PerObjectRange.OffsetInDescriptorsFromTableStart = D3D12_DESCRIPTOR_RANGE_OFFSET_APPEND;

        D3D12_ROOT_DESCRIPTOR_TABLE PerPassRootDescriptorTable = {};
        PerPassRootDescriptorTable.NumDescriptorRanges = 1;
        PerPassRootDescriptorTable.pDescriptorRanges = &PerPassRange;

        D3D12_ROOT_DESCRIPTOR_TABLE PerObjectRootDescriptorTable = {};
        PerObjectRootDescriptorTable.NumDescriptorRanges = 1;
        PerObjectRootDescriptorTable.pDescriptorRanges = &PerObjectRange;

        D3D12_ROOT_DESCRIPTOR TileRootDescriptor;
        TileRootDescriptor.ShaderRegister = 2;
        TileRootDescriptor.RegisterSpace = 0;

        D3D12_ROOT_PARAMETER SlotRootParameters[3];
        SlotRootParameters[0].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        SlotRootParameters[0].DescriptorTable = PerPassRootDescriptorTable;
        SlotRootParameters[0].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        SlotRootParameters[1].ParameterType = D3D12_ROOT_PARAMETER_TYPE_DESCRIPTOR_TABLE;
        SlotRootParameters[1].DescriptorTable = PerObjectRootDescriptorTable;
        SlotRootParameters[1].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;
        SlotRootParameters[2].ParameterType = D3D12_ROOT_PARAMETER_TYPE_CBV;
        SlotRootParameters[2].Descriptor = TileRootDescriptor;
        SlotRootParameters[2].ShaderVisibility = D3D12_SHADER_VISIBILITY_ALL;

        D3D12_ROOT_SIGNATURE_DESC RootSignatureDesc = {};
        RootSignatureDesc.NumParameters = 3;
        RootSignatureDesc.pParameters = SlotRootParameters;
        RootSignatureDesc.NumStaticSamplers = 0;
        RootSignatureDesc.pStaticSamplers = NULL;
        RootSignatureDesc.Flags = D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT;

        Microsoft::WRL::ComPtr<ID3DBlob> SerializedRootSignature;
        Hr = D3D12SerializeRootSignature(&RootSignatureDesc,
                                         D3D_ROOT_SIGNATURE_VERSION_1,
                                         SerializedRootSignature.GetAddressOf(),
                                         NULL);
        ASSERT(SUCCEEDED(Hr));

        Hr = Device->CreateRootSignature(0,
                                         SerializedRootSignature->GetBufferPointer(),
                                         SerializedRootSignature->GetBufferSize(),
                                         IID_PPV_ARGS(RootSignature.GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // Create PSO for rects
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};

        Desc.pRootSignature = RootSignature.Get();

        {
            debug_ReadFileResult VSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_rect.cso"
            Memory->debug_platformReadEntireFile(NULL, &VSFile, FILENAME);
#undef FILENAME
            ASSERT(VSFile.contentsSize);

            Desc.VS = { VSFile.contents, VSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &VSFile);
        }

        {
            debug_ReadFileResult PSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_rect.cso"
            Memory->debug_platformReadEntireFile(NULL, &PSFile, FILENAME);
#undef FILENAME
            ASSERT(PSFile.contentsSize);

            Desc.PS = { PSFile.contents, PSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &PSFile);
        }

        Desc.RasterizerState = RasterizerDesc;
        Desc.BlendState = BlendDesc;

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

            Desc.DepthStencilState = DepthStencilDesc;
        }

        Desc.SampleMask = UINT_MAX;
        Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        Desc.NumRenderTargets = 1;
        Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        Hr = Device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(PipelineState[RECT_PIPELINE_STATE_IDX].GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // Create PSO for lines
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};

        {
            D3D12_INPUT_ELEMENT_DESC InputLayout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                                                       { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } };

            Desc.InputLayout = { InputLayout, ARRAY_COUNT(InputLayout) };
        }

        Desc.pRootSignature = RootSignature.Get();

        {
            debug_ReadFileResult VSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_line.cso"
            Memory->debug_platformReadEntireFile(NULL, &VSFile, FILENAME);
#undef FILENAME
            ASSERT(VSFile.contentsSize);

            Desc.VS = { VSFile.contents, VSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &VSFile);
        }

        {
            debug_ReadFileResult PSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_line.cso"
            Memory->debug_platformReadEntireFile(NULL, &PSFile, FILENAME);
#undef FILENAME
            ASSERT(PSFile.contentsSize);

            Desc.PS = { PSFile.contents, PSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &PSFile);
        }

        Desc.RasterizerState = RasterizerDesc;
        Desc.BlendState = BlendDesc;

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

            Desc.DepthStencilState = DepthStencilDesc;
        }

        Desc.SampleMask = UINT_MAX;
        Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
        Desc.NumRenderTargets = 1;
        Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        Hr = Device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(PipelineState[LINE_PIPELINE_STATE_IDX].GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // Create PSO for points
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};

        {
            D3D12_INPUT_ELEMENT_DESC InputLayout[] = { { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                                                       { "COLOR", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 } };

            Desc.InputLayout = { InputLayout, ARRAY_COUNT(InputLayout) };
        }

        Desc.pRootSignature = RootSignature.Get();

        {
            debug_ReadFileResult VSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_point.cso"
            Memory->debug_platformReadEntireFile(NULL, &VSFile, FILENAME);
#undef FILENAME
            ASSERT(VSFile.contentsSize);

            Desc.VS = { VSFile.contents, VSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &VSFile);
        }

        {
            debug_ReadFileResult PSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_point.cso"
            Memory->debug_platformReadEntireFile(NULL, &PSFile, FILENAME);
#undef FILENAME
            ASSERT(PSFile.contentsSize);

            Desc.PS = { PSFile.contents, PSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &PSFile);
        }

        Desc.RasterizerState = RasterizerDesc;
        Desc.RasterizerState.FillMode = D3D12_FILL_MODE_SOLID;
        Desc.BlendState = BlendDesc;

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

            Desc.DepthStencilState = DepthStencilDesc;
        }

        Desc.SampleMask = UINT_MAX;
        Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_POINT;
        Desc.NumRenderTargets = 1;
        Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        Hr = Device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(PipelineState[POINT_PIPELINE_STATE_IDX].GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // NOTE(dima): create PSO for tiles
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};

        Desc.pRootSignature = RootSignature.Get();

        {
            debug_ReadFileResult VSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_tile.cso"
            Memory->debug_platformReadEntireFile(NULL, &VSFile, FILENAME);
#undef FILENAME
            ASSERT(VSFile.contentsSize);

            Desc.VS = { VSFile.contents, VSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &VSFile);
        }

        {
            debug_ReadFileResult PSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_tile.cso"
            Memory->debug_platformReadEntireFile(NULL, &PSFile, FILENAME);
#undef FILENAME
            ASSERT(PSFile.contentsSize);

            Desc.PS = { PSFile.contents, PSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &PSFile);
        }

        Desc.RasterizerState = RasterizerDesc;
        Desc.BlendState = BlendDesc;

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

            Desc.DepthStencilState = DepthStencilDesc;
        }

        Desc.SampleMask = UINT_MAX;
        Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        Desc.NumRenderTargets = 1;
        Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        Hr = Device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(PipelineState[TILE_PIPELINE_STATE_IDX].GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }

    // NOTE(dima): create PSO for debugging textures
    {
        D3D12_GRAPHICS_PIPELINE_STATE_DESC Desc = {};

        Desc.pRootSignature = RootSignature.Get();

        {
            debug_ReadFileResult VSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_vshader_texture_debug.cso"
            Memory->debug_platformReadEntireFile(NULL, &VSFile, FILENAME);
#undef FILENAME
            ASSERT(VSFile.contentsSize);

            Desc.VS = { VSFile.contents, VSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &VSFile);
        }

        {
            debug_ReadFileResult PSFile = {};
#define FILENAME "build" DIR_SEPARATOR "d3d11_pshader_texture_debug.cso"
            Memory->debug_platformReadEntireFile(NULL, &PSFile, FILENAME);
#undef FILENAME
            ASSERT(PSFile.contentsSize);

            Desc.PS = { PSFile.contents, PSFile.contentsSize };

            Memory->debug_platformFreeFileMemory(NULL, &PSFile);
        }

        Desc.RasterizerState = RasterizerDesc;
        Desc.BlendState = BlendDesc;

        {
            D3D12_DEPTH_STENCILOP_DESC DefaultStencilOp =  { D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_STENCIL_OP_KEEP, D3D12_COMPARISON_FUNC_ALWAYS };

            D3D12_DEPTH_STENCIL_DESC DepthStencilDesc = {};
            DepthStencilDesc.DepthEnable = false;
            DepthStencilDesc.DepthWriteMask = D3D12_DEPTH_WRITE_MASK_ALL;
            DepthStencilDesc.DepthFunc = D3D12_COMPARISON_FUNC_LESS;
            DepthStencilDesc.StencilEnable = false;
            DepthStencilDesc.StencilReadMask = D3D12_DEFAULT_STENCIL_READ_MASK;
            DepthStencilDesc.StencilWriteMask = D3D12_DEFAULT_STENCIL_WRITE_MASK;
            DepthStencilDesc.FrontFace = DefaultStencilOp;
            DepthStencilDesc.BackFace = DefaultStencilOp;

            Desc.DepthStencilState = DepthStencilDesc;
        }

        Desc.SampleMask = UINT_MAX;
        Desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
        Desc.NumRenderTargets = 1;
        Desc.RTVFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;
        Desc.SampleDesc.Count = 1;
        Desc.SampleDesc.Quality = 0;
        Desc.DSVFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;

        Hr = Device->CreateGraphicsPipelineState(&Desc, IID_PPV_ARGS(PipelineState[TEXTURE_DEBUG_PIPELINE_STATE_IDX].GetAddressOf()));
        ASSERT(SUCCEEDED(Hr));
    }
}

RENDER_ON_GPU(renderOnGPU)
{
    CmdAlloc->Reset();
    CmdList->Reset(CmdAlloc.Get(), 0);

    Microsoft::WRL::ComPtr<ID3D12Resource> CurrentBackBuffer = SwapChainBuffer[CurrentBackBufferIdx];

    // NOTE(dima): transition current back buffer state
    {
        D3D12_RESOURCE_TRANSITION_BARRIER TransitionBarrier = {};
        TransitionBarrier.pResource = CurrentBackBuffer.Get();
        TransitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        TransitionBarrier.StateBefore = D3D12_RESOURCE_STATE_PRESENT;
        TransitionBarrier.StateAfter = D3D12_RESOURCE_STATE_RENDER_TARGET;

        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition = TransitionBarrier;

        CmdList->ResourceBarrier(1, &Barrier);
    }

    CmdList->RSSetViewports(1, &Viewport);
    CmdList->RSSetScissorRects(1, &ScissorRect);

    renderGroupVBCurrentSize = 0;
    Current_CBV_SRV_UAV_DescriptorIdx = 0;
    CurrentPerObjectCBOffset = 0;

#if 0
    HRESULT Hr;
#endif

    D3D12_CPU_DESCRIPTOR_HANDLE CurrHndlRTV = GetCPUDescriptorHandle(RTVHeap,
                                                                     RTVDescriptorSize,
                                                                     CurrentBackBufferIdx);
    r32 RTVClearColor[4] = {0.3f, 0.3f, 0.3f, 1.0f};
    CmdList->ClearRenderTargetView(CurrHndlRTV, RTVClearColor, 0, NULL);

    D3D12_CPU_DESCRIPTOR_HANDLE CurrHndlDSV = DSVHeap->GetCPUDescriptorHandleForHeapStart();
    CmdList->ClearDepthStencilView(CurrHndlDSV,
                                   D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL,
                                   1.0f,
                                   0,
                                   0,
                                   NULL);

    CmdList->OMSetRenderTargets(1, &CurrHndlRTV, true, &CurrHndlDSV);

    ID3D12DescriptorHeap* CBV_SRV_UAV_Heaps[] = { CBV_SRV_UAV_Heap.Get() };
    CmdList->SetDescriptorHeaps(ARRAY_COUNT(CBV_SRV_UAV_Heaps), CBV_SRV_UAV_Heaps);

    CmdList->SetGraphicsRootSignature(RootSignature.Get());

    D3D12_GPU_DESCRIPTOR_HANDLE PerPassCBVGPUHndl = GetGPUDescriptorHandle(CBV_SRV_UAV_Heap,
                                                                           CBV_SRV_UAV_DescriptorSize,
                                                                           0);
    CmdList->SetGraphicsRootDescriptorTable(0, PerPassCBVGPUHndl);

    // NOTE(dima): create texture
    {
#if 0
#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"
        debug_ReadFileResult TTFFile;
#define FILENAME "C:/Windows/Fonts/arial.ttf"
        Memory->debug_platformReadEntireFile(NULL, &TTFFile, FILENAME);
#undef FILENAME
        ASSERT(TTFFile.contentsSize != 0);

        stbtt_fontinfo Font;
        stbtt_InitFont(&Font, (u8 *)TTFFile.contents, stbtt_GetFontOffsetForIndex((u8 *)TTFFile.contents, 0));
        s32 Width, Height, XOffset, YOffset;
        u8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 128.0f), 'n', &Width, &Height, &XOffset, &YOffset);

        D3D12_RESOURCE_DESC TextureDesc = {};
        TextureDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
        TextureDesc.Alignment = 0;
        TextureDesc.Width = Width;
        TextureDesc.Height = Height;
        TextureDesc.DepthOrArraySize = 1;
        TextureDesc.MipLevels = 1;
        TextureDesc.Format = DXGI_FORMAT_R32G32B32A32_UINT;
        TextureDesc.SampleDesc.Count = 1;
        TextureDesc.SampleDesc.Quality = 0;
        TextureDesc.Layout = D3D12_TEXTURE_LAYOUT_ROW_MAJOR; 
        TextureDesc.Flags = D3D12_RESOURCE_FLAG_NONE;

        D3D12_HEAP_PROPERTIES HeapProperties = {};
        HeapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
        HeapProperties.CPUPageProperty = D3D12_CPU_PAGE_PROPERTY_UNKNOWN;
        HeapProperties.MemoryPoolPreference = D3D12_MEMORY_POOL_UNKNOWN;
        HeapProperties.CreationNodeMask = 1;
        HeapProperties.VisibleNodeMask = 1;

        HRESULT Hr = Device->CreateCommittedResource(&HeapProperties,
                                                     D3D12_HEAP_FLAG_NONE,
                                                     &TextureDesc,
                                                     D3D12_RESOURCE_STATE_COPY_DEST,
                                                     NULL,
                                                     IID_PPV_ARGS(&Texture));
        ASSERT(SUCCEEDED(Hr));

        D3D12_TEX2D_SRV TexSRV =  {};
        TexSRV.MostDetailedMip = 0;
        TexSRV.MipLevels = 1;

        D3D12_SHADER_RESOURCE_VIEW_DESC Desc = {};
        Desc.Format = TextureDesc.Format;
        Desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
        Desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
        Desc.Texture2D = TexSRV;

        D3D12_CPU_DESCRIPTOR_HANDLE CurrHndl = GetCPUDescriptorHandle(CBV_SRV_UAV_Heap,
                                                                     CBV_SRV_UAV_DescriptorSize,
                                                                     Current_CBV_SRV_UAV_DescriptorIdx);
        ++Current_CBV_SRV_UAV_DescriptorIdx;

        Device->CreateShaderResourceView(Texture,
                                         &Desc,
                                         CurrHndl);

        stbtt_FreeBitmap(MonoBitmap, NULL);
#endif
    }

    // NOTE(dima): populate per-pass CB
    {
#if 0
        u8 *MappedData;
        Hr = RenderGroupPerPassCB->Map(0, NULL, (void **)&MappedData);
        ASSERT(SUCCEEDED(Hr));

        u32 DataLength = sizeof(renderGroup->uniforms);
        memcpy(MappedData, renderGroup->uniforms, DataLength);
        memcpy(MappedData + DataLength, &Viewport.Width, sizeof(Viewport.Width));
        memcpy(MappedData + DataLength + sizeof(Viewport.Width), &Viewport.Height, sizeof(Viewport.Height));

        RenderGroupPerPassCB->Unmap(0, NULL);

        u32 SizeOfData = RoundToNearestMultipleOf256(sizeof(renderGroup->uniforms) + sizeof(Viewport.Width) + sizeof(Viewport.Height));
        CreateConstantBufferView(RenderGroupPerPassCB,
                                 CBV_SRV_UAV_Heap,
                                 CBV_SRV_UAV_DescriptorSize,
                                 Current_CBV_SRV_UAV_DescriptorIdx,
                                 0,
                                 SizeOfData);
        ++Current_CBV_SRV_UAV_DescriptorIdx;
#endif
    }

    // NOTE(dima): draw render items
#if 0
    RenderGroupEntryHeader *EntryHeader = (RenderGroupEntryHeader *)renderGroup->pushBufferBase;
    while (EntryHeader)
    {
        ASSERT(renderGroupVBCurrentSize < MAX_RENDER_GROUP_VB_SIZE);
        ASSERT(renderGroupCBCurrentSize < MAX_RENDER_GROUP_PER_OBJECT_CB_SIZE);
        switch (EntryHeader->type)
        {
            case RenderGroupEntryType_RenderEntryClear:
            {
                r32 RTVClearColor[4] = {0.3f, 0.3f, 0.3f, 1.0f};
                CmdList->ClearRenderTargetView(CurrHndlRTV, RTVClearColor, 0, NULL);

                break;
            }
            case RenderGroupEntryType_RenderEntryPoint:
            {
                CmdList->SetPipelineState(PipelineState[POINT_PIPELINE_STATE_IDX].Get());

                RenderEntryPoint *Entry = (RenderEntryPoint *)(EntryHeader + 1);

                u32 renderGroupVBStartOffset = renderGroupVBCurrentSize;
                {
                    r32 vertices[4 * 3 * 2];
                    V4 pointPosCamera = v4(Entry->position.x,
                                           Entry->position.y,
                                           Entry->position.z,
                                           1.0f);
                    memcpy(vertices, (void*)&Entry->position, sizeof(V3));
                    memcpy(vertices + 3, (void*)&Entry->color, sizeof(Entry->color));
                    memcpy(vertices + 6, (void*)&Entry->position, sizeof(V3));
                    memcpy(vertices + 9, (void*)&Entry->color, sizeof(Entry->color));
                    memcpy(vertices + 12, (void*)&Entry->position, sizeof(V3));
                    memcpy(vertices + 15, (void*)&Entry->color, sizeof(Entry->color));
                    memcpy(vertices + 18, (void*)&Entry->position, sizeof(V3));
                    memcpy(vertices + 21, (void*)&Entry->color, sizeof(Entry->color));

                    u32 sizeOfData = sizeof(r32)*ARRAY_COUNT(vertices);
                    renderGroupVBCurrentSize += sizeOfData;

                    u8 *MappedData;
                    Hr = RenderGroupVB->Map(0, NULL, (void **)&MappedData);
                    ASSERT(SUCCEEDED(Hr));
                    MappedData += renderGroupVBStartOffset;

                    memcpy(MappedData, vertices, sizeOfData);

                    RenderGroupVB->Unmap(0, NULL);

                    D3D12_VERTEX_BUFFER_VIEW View;
                    View.BufferLocation = RenderGroupVB->GetGPUVirtualAddress() + renderGroupVBStartOffset;
                    View.SizeInBytes = renderGroupVBCurrentSize - renderGroupVBStartOffset;
                    // TODO(dima): currently, will have to modify this every time InputLayout changes
                    View.StrideInBytes = 2*sizeof(V3);

                    CmdList->IASetVertexBuffers(0, 1, &View);
                }

                CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                CmdList->DrawInstanced(4, 1, 0, 0);

                break;
            }
            case RenderGroupEntryType_RenderEntryLine:
            {
                CmdList->SetPipelineState(PipelineState[LINE_PIPELINE_STATE_IDX].Get());

                RenderEntryLine *Entry = (RenderEntryLine *)(EntryHeader + 1);

                u32 renderGroupVBStartOffset = renderGroupVBCurrentSize;
                {
                    r32 vertices[2 * 3 * 2];
                    memcpy(vertices, (void*)&Entry->start, sizeof(Entry->start));
                    memcpy(vertices + 3, (void*)&Entry->color, sizeof(Entry->color));
                    memcpy(vertices + 6, (void*)&Entry->end, sizeof(Entry->end));
                    memcpy(vertices + 9, (void*)&Entry->color, sizeof(Entry->color));

                    u32 sizeOfData = sizeof(r32)*ARRAY_COUNT(vertices);
                    renderGroupVBCurrentSize += sizeOfData;

                    u8 *MappedData;
                    Hr = RenderGroupVB->Map(0, NULL, (void **)&MappedData);
                    ASSERT(SUCCEEDED(Hr));
                    MappedData += renderGroupVBStartOffset;

                    memcpy(MappedData, vertices, sizeOfData);

                    RenderGroupVB->Unmap(0, NULL);

                    D3D12_VERTEX_BUFFER_VIEW View;
                    View.BufferLocation = RenderGroupVB->GetGPUVirtualAddress() + renderGroupVBStartOffset;
                    View.SizeInBytes = renderGroupVBCurrentSize - renderGroupVBStartOffset;
                    // TODO(dima): currently, will have to modify this every time InputLayout changes
                    View.StrideInBytes = 2*sizeof(V3);

                    CmdList->IASetVertexBuffers(0, 1, &View);
                }

                CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);

                CmdList->DrawInstanced(2, 1, 0, 0);

                break;
            }
            case RenderGroupEntryType_RenderEntryTile:
            {
                CmdList->SetPipelineState(PipelineState[TILE_PIPELINE_STATE_IDX].Get());

                RenderEntryTile *Entry = (RenderEntryTile *)(EntryHeader + 1);

                D3D12_GPU_VIRTUAL_ADDRESS CBAddress = RenderGroupPerObjectCB->GetGPUVirtualAddress() 
                                                      + CurrentPerObjectCBOffset;
                CmdList->SetGraphicsRootConstantBufferView(2, CBAddress);

                {
                    u32 LengthOfTileCountPerSide = sizeof(Entry->tileCountPerSide);
                    u32 LengthOfTileSideLength = sizeof(Entry->tileSideLength);
                    u32 LengthOfTileColor = sizeof(Entry->color);
                    u32 LengthOfOriginTileCenterPositionWorld = sizeof(Entry->originTileCenterPositionWorld);
                    u32 TotalSize = RoundToNearestMultipleOf256(LengthOfTileCountPerSide
                                                                + LengthOfTileSideLength
                                                                + LengthOfTileColor
                                                                + LengthOfOriginTileCenterPositionWorld);
                    u8 *MappedData;
                    Hr = RenderGroupPerObjectCB->Map(0, NULL, (void **)&MappedData);
                    ASSERT(SUCCEEDED(Hr));

                    MappedData += CurrentPerObjectCBOffset;
                    memcpy(MappedData, &Entry->color, LengthOfTileColor);
                    MappedData += LengthOfTileColor
                                  // NOTE(dima): padding to respect HLSL constant buffer's memory alignment rules
                                  + sizeof(r32);
                    memcpy(MappedData, &Entry->originTileCenterPositionWorld, LengthOfOriginTileCenterPositionWorld);
                    MappedData += LengthOfOriginTileCenterPositionWorld;
                    memcpy(MappedData, &Entry->tileCountPerSide, LengthOfTileCountPerSide);
                    MappedData += LengthOfTileCountPerSide;
                    memcpy(MappedData, &Entry->tileSideLength, LengthOfTileSideLength);

                    RenderGroupPerObjectCB->Unmap(0, NULL);

                    // NOTE(dima): create CBV for the data we just copied
                    CreateConstantBufferView(RenderGroupPerObjectCB,
                                             CBV_SRV_UAV_Heap,
                                             CBV_SRV_UAV_DescriptorSize,
                                             Current_CBV_SRV_UAV_DescriptorIdx,
                                             CurrentPerObjectCBOffset,
                                             TotalSize);
                    ++Current_CBV_SRV_UAV_DescriptorIdx;

                    CurrentPerObjectCBOffset += TotalSize;
                }

                CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                CmdList->DrawInstanced(4, Entry->tileCountPerSide * Entry->tileCountPerSide, 0, 0);

            } break;
            case RenderGroupEntryType_RenderEntryRect:
            {
                CmdList->SetPipelineState(PipelineState[RECT_PIPELINE_STATE_IDX].Get());

                RenderEntryRect *Entry = (RenderEntryRect *)(EntryHeader + 1);

                D3D12_GPU_DESCRIPTOR_HANDLE PerObjectCBVGPUHndl = GetGPUDescriptorHandle(CBV_SRV_UAV_Heap, CBV_SRV_UAV_DescriptorSize, Current_CBV_SRV_UAV_DescriptorIdx);
                CmdList->SetGraphicsRootDescriptorTable(1, PerObjectCBVGPUHndl);

                {
                    u32 SideLengthWLength = sizeof(Entry->sideLengthW);
                    u32 SideLengthHLength = sizeof(Entry->sideLengthH);
                    u32 RectCenterPositionWorldLength = sizeof(Entry->rectCenterPositionWorld);
                    u32 ColorLength = sizeof(Entry->color);
                    u32 TotalSize = RoundToNearestMultipleOf256(SideLengthWLength
                                                                + SideLengthHLength
                                                                + RectCenterPositionWorldLength
                                                                + ColorLength);

                    u8 *MappedData;
                    Hr = RenderGroupPerObjectCB->Map(0, NULL, (void **)&MappedData);
                    ASSERT(SUCCEEDED(Hr));

                    MappedData += CurrentPerObjectCBOffset;

                    memcpy(MappedData, &Entry->rectCenterPositionWorld, RectCenterPositionWorldLength);
                    MappedData += RectCenterPositionWorldLength;

                    memcpy(MappedData, &Entry->sideLengthW, SideLengthWLength);
                    MappedData += SideLengthWLength;

                    memcpy(MappedData, &Entry->color, ColorLength);
                    MappedData += ColorLength;

                    memcpy(MappedData, &Entry->sideLengthH, SideLengthHLength);

                    RenderGroupPerObjectCB->Unmap(0, NULL);

                    // NOTE(dima): create CBV for the data we just copied
                    CreateConstantBufferView(RenderGroupPerObjectCB.Get(),
                                             CBV_SRV_UAV_Heap,
                                             CBV_SRV_UAV_DescriptorSize,
                                             Current_CBV_SRV_UAV_DescriptorIdx,
                                             CurrentPerObjectCBOffset,
                                             TotalSize);
                    ++Current_CBV_SRV_UAV_DescriptorIdx;

                    CurrentPerObjectCBOffset += TotalSize;
                }

                CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                CmdList->DrawInstanced(4, 1, 0, 0);

                break;
            }
            case RenderGroupEntryType_RenderEntryTextureDebug:
            {
                CmdList->SetPipelineState(PipelineState[TEXTURE_DEBUG_PIPELINE_STATE_IDX].Get());

                RenderEntryTextureDebug *Entry = (RenderEntryTextureDebug *)(EntryHeader + 1);

                CmdList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

                CmdList->DrawInstanced(4, 1, 0, 0);

                break;
            }
            default:
                break;
        }

        EntryHeader = EntryHeader->next;
    }
#endif

    // NOTE(dima): transition current back buffer state
    {
        D3D12_RESOURCE_TRANSITION_BARRIER TransitionBarrier = {};
        TransitionBarrier.pResource = CurrentBackBuffer.Get();
        TransitionBarrier.Subresource = D3D12_RESOURCE_BARRIER_ALL_SUBRESOURCES;
        TransitionBarrier.StateBefore = D3D12_RESOURCE_STATE_RENDER_TARGET;
        TransitionBarrier.StateAfter = D3D12_RESOURCE_STATE_PRESENT;

        D3D12_RESOURCE_BARRIER Barrier = {};
        Barrier.Type = D3D12_RESOURCE_BARRIER_TYPE_TRANSITION;
        Barrier.Flags = D3D12_RESOURCE_BARRIER_FLAG_NONE;
        Barrier.Transition = TransitionBarrier;

        CmdList->ResourceBarrier(1, &Barrier);
    }

    CmdList->Close();

    ID3D12CommandList* CmdLists[] = { CmdList.Get() };
    CmdQueue->ExecuteCommandLists(1, CmdLists);

    if (TearingSupportEnabled())
    {
        SwapChain->Present(1, 0);
    }
    else
    {
        SwapChain->Present(0, 0);
    }

    CurrentBackBufferIdx = (CurrentBackBufferIdx + 1) % SWAP_CHAIN_BUFFER_COUNT;

    FlushCommandQueue();
}

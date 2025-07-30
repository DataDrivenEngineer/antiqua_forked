#ifndef _WIN32_ANTIQUA_H_
#include <windows.h>
#include <wrl.h>

#include "antiqua_platform.h"

using Microsoft::WRL::ComPtr;

typedef struct State
{
    GameMemory *gameMemory;

    HWND window;

    u32 windowWidth;
    u32 windowHeight;
    b32 windowedMode;
    b32 windowVisible;

} State;

typedef struct AntiquaTexture
{
    ComPtr<ID3D12Resource> gpuTexture;
    b32 initialized;
    b32 needsGpuReupload;
    D3D12_RESOURCE_STATES state;
} AntiquaTexture;

typedef struct AntiquaTextureUploadHeap
{
    ComPtr<ID3D12Resource> gpuUploadHeap;
    b32 initialized;
    u32 currentTextureUploadHeapOffset;
    u32 framesPassedSinceWritingToBeginningOfHeap;
} AntiquaTextureUploadHeap;

internal void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

#define _WIN32_ANTIQUA_H_
#endif

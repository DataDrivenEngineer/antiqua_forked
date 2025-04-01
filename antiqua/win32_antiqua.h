#ifndef _WIN32_ANTIQUA_H_
#include <windows.h>

#include "antiqua_platform.h"

typedef struct State
{
  u64 totalMemorySize;
  void *gameMemoryBlock;

  s32 recordingHandle;
  s32 inputRecordingIndex;
  s32 playBackHandle;
  s32 inputPlayingIndex;

  b32 toggleFullscreen;
} State;

typedef struct InitRendererData
{
    HWND Window;
    u32 WindowWidth;
    u32 WindowHeight;
    GameMemory *Memory;
} InitRendererData;

internal void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

#define _WIN32_ANTIQUA_H_
#endif

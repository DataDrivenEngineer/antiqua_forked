#ifndef _WIN32_ANTIQUA_H_
#include <windows.h>

#include "antiqua_platform.h"

typedef struct State
{
    u64 totalMemorySize;
    void *gameMemoryBlock;

    HWND window;

    u32 windowWidth;
    u32 windowHeight;
    b32 windowedMode;
    b32 windowVisible;

} State;

internal void FatalError(const char* message)
{
    MessageBoxA(NULL, message, "Error", MB_ICONEXCLAMATION);
    ExitProcess(0);
}

#define _WIN32_ANTIQUA_H_
#endif

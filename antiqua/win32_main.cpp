#include <windows.h>

#include "win32_antiqua.h"
#include "antiqua_platform.h"

#define DEBUG_CONSOLE_ENABLED 1
#define DISPLAY_FRAME_TIME 1

internal b32 GlobalRunning;

internal State win32State = {};

DEBUG_PLATFORM_FREE_FILE_MEMORY(DEBUGPlatformFreeFileMemory)
{
    if(file)
    {
        VirtualFree(file, 0, MEM_RELEASE);
    }
}

DEBUG_PLATFORM_READ_ENTIRE_FILE(DEBUGPlatformReadEntireFile)
{
    b32 Result = false;

    s8 currDir[1024];
    GetCurrentDirectory(
      1024,
      (LPSTR)currDir
    );
    
    HANDLE FileHandle = CreateFileA(filename, GENERIC_READ, FILE_SHARE_READ, 0, OPEN_EXISTING, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        LARGE_INTEGER FileSize;
        if(GetFileSizeEx(FileHandle, &FileSize))
        {
            u32 FileSize32 = safeTruncateUInt64(FileSize.QuadPart);
            outFile->contents = VirtualAlloc(0, FileSize32, MEM_RESERVE|MEM_COMMIT, PAGE_READWRITE);
            if(outFile->contents)
            {
                DWORD BytesRead;
                if(ReadFile(FileHandle, outFile->contents, FileSize32, &BytesRead, 0) &&
                   (FileSize32 == BytesRead))
                {
                    // NOTE(casey): File read successfully
                    outFile->contentsSize = FileSize32;
                    Result = true;
                }
                else
                {                    
                    // TODO(casey): Logging
                    DEBUGPlatformFreeFileMemory(thread, (debug_ReadFileResult *)outFile->contents);
                    outFile->contents = 0;
                }
            }
            else
            {
                // TODO(casey): Logging
            }
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(Result);
}

DEBUG_PLATFORM_WRITE_ENTIRE_FILE(DEBUGPlatformWriteEntireFile)
{
    b32 Result = false;
    
    HANDLE FileHandle = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
    if(FileHandle != INVALID_HANDLE_VALUE)
    {
        DWORD BytesWritten;
        if(WriteFile(FileHandle, memory, memorySize, &BytesWritten, 0))
        {
            // NOTE(casey): File read successfully
            Result = (BytesWritten == memorySize);
        }
        else
        {
            // TODO(casey): Logging
        }

        CloseHandle(FileHandle);
    }
    else
    {
        // TODO(casey): Logging
    }

    return(Result);
}


internal void Win32ProcessKeyboardMessage(GameButtonState *NewState, b32 IsDown)
{
    if(NewState->endedDown != IsDown)
    {
        NewState->endedDown = IsDown;
        ++NewState->halfTransitionCount;
    }
}

internal void Win32ProcessPendingMessages(GameControllerInput *NewInput)
{
    MSG Message;
    while(PeekMessage(&Message, 0, 0, 0, PM_REMOVE))
    {
        switch(Message.message)
        {
            case WM_QUIT:
            {
                GlobalRunning = false;
            } break;

            case WM_INPUT:
            {
                u32 dwSize;
                GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, NULL, &dwSize, sizeof(RAWINPUTHEADER));
                u8 *lpb = new u8[dwSize];
                ASSERT(GetRawInputData((HRAWINPUT)Message.lParam, RID_INPUT, lpb, &dwSize, sizeof(RAWINPUTHEADER)) == dwSize);

                RAWINPUT* raw = (RAWINPUT*)lpb;

                if (raw->header.dwType == RIM_TYPEMOUSE) 
                {
                    if (raw->data.mouse.usButtonFlags & RI_MOUSE_WHEEL)
                    {
                        NewInput->scrollingDeltaY = raw->data.mouse.usButtonData < 32768 ? -1 : 1;
                    }
                } else if (raw->header.dwType == RIM_TYPEKEYBOARD)
                {
                    u32 VKCode = raw->data.keyboard.VKey;
                    b32 IsDown = raw->data.keyboard.Flags & RI_KEY_BREAK ? 0 : 1;

                    if(VKCode == 'W')
                    {
                        Win32ProcessKeyboardMessage(&NewInput->up, IsDown);
                    }
                    else if(VKCode == 'A')
                    {
                        Win32ProcessKeyboardMessage(&NewInput->left, IsDown);
                    }
                    else if(VKCode == 'S')
                    {
                        Win32ProcessKeyboardMessage(&NewInput->down, IsDown);
                    }
                    else if(VKCode == 'D')
                    {
                        Win32ProcessKeyboardMessage(&NewInput->right, IsDown);
                    }
                    else if(VKCode == 'Q')
                    {
                    }
                    else if(VKCode == 'E')
                    {
                    }
                    else if(VKCode == VK_UP)
                    {
                        Win32ProcessKeyboardMessage(&NewInput->actionUp, IsDown);
                    }
                    else if(VKCode == VK_LEFT)
                    {
                        Win32ProcessKeyboardMessage(&NewInput->actionLeft, IsDown);
                    }
                    else if(VKCode == VK_DOWN)
                    {
                        Win32ProcessKeyboardMessage(&NewInput->actionDown, IsDown);
                    }
                    else if(VKCode == VK_RIGHT)
                    {
                        Win32ProcessKeyboardMessage(&NewInput->actionRight, IsDown);
                    }
                    else if(VKCode == VK_ESCAPE)
                    {
                    }
                    else if(VKCode == VK_SPACE)
                    {
                    }

#if 0
                    b32 LeftAltKeyIsDown = raw->data.keyboard.MakeCode & 0x0038;
                    if((VKCode == VK_F3) && LeftAltKeyIsDown)
                    {
                        GlobalRunning = false;
                    }
#endif
                }

                delete[] lpb;
            } break;
            
            default:
            {
                TranslateMessage(&Message);
                DispatchMessageA(&Message);
            } break;
        }
    }
}

internal LRESULT CALLBACK WindowProc(HWND Wnd, UINT Msg, WPARAM WParam, LPARAM LParam)
{
    switch (Msg)
    {
        case WM_EXITSIZEMOVE:
        {
            ResizeWindow(&Wnd);
        } break;

        case WM_DESTROY:
        case WM_CLOSE:
        {
            GlobalRunning = false;
        } break;
    }

    return DefWindowProcW(Wnd, Msg, WParam, LParam);
}

s32 WINAPI WinMain(HINSTANCE Instance, HINSTANCE PrevInstance, LPSTR CmdLine, s32 CmdShow)
{
    WNDCLASSEXW WndCls = {};
    WndCls.style         = CS_HREDRAW | CS_VREDRAW;
    WndCls.cbSize        = sizeof(WndCls);
    WndCls.lpfnWndProc   = WindowProc;
    WndCls.cbClsExtra    = 0;
	WndCls.cbWndExtra    = 0;
	WndCls.hInstance     = Instance;
	WndCls.hIcon         = LoadIcon(0, IDI_APPLICATION);
	WndCls.hCursor       = LoadCursor(0, IDC_ARROW);
	WndCls.hbrBackground = (HBRUSH)(COLOR_WINDOW + 1);
	WndCls.lpszMenuName  = 0;
	WndCls.lpszClassName = L"MainWnd";
    
    if (!RegisterClassExW(&WndCls))
    {
        u32 err = GetLastError();
        MessageBox(0, "RegisterClass Failed.", 0, 0);
        return -1;
    }

    // Compute window rectangle dimensions based on requested client area dimensions.
	RECT R = { 0, 0, 1920, 1080 };
    AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);
	s32 Width  = R.right - R.left;
	s32 Height = R.bottom - R.top;

	HWND Window = CreateWindowExW(WS_EX_APPWINDOW | WS_EX_NOREDIRECTIONBITMAP,
                                 WndCls.lpszClassName,
                                 L"Antiqua",
                                 WS_OVERLAPPEDWINDOW,
                                 CW_USEDEFAULT,
                                 CW_USEDEFAULT,
                                 Width,
                                 Height,
                                 NULL,
                                 NULL,
                                 WndCls.hInstance,
                                 NULL);
		
	if(!Window)
	{
		MessageBox(0, "CreateWindow Failed.", 0, 0);
		return -1;
	}

	ShowWindow(Window, SW_SHOW);

    GameMemory gameMemory = {};
    gameMemory.permanentStorageSize = GB(1);
    gameMemory.transientStorageSize = GB(1);
    gameMemory.debug_platformFreeFileMemory = DEBUGPlatformFreeFileMemory;
    gameMemory.debug_platformReadEntireFile = DEBUGPlatformReadEntireFile;
    gameMemory.debug_platformWriteEntireFile = DEBUGPlatformWriteEntireFile;

#if ANTIQUA_INTERNAL
    LPVOID baseAddress = (LPVOID)GB(2*1024);
#else
    LPVOID baseAddress = 0;
#endif
    // TODO(casey): Handle various memory footprints (USING SYSTEM METRICS)
    // TODO(casey): Use MEM_LARGE_PAGES and call adjust token
    // privileges when not on Windows XP?
    win32State.totalMemorySize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
    win32State.gameMemoryBlock = VirtualAlloc(baseAddress,
                                              (size_t)win32State.totalMemorySize,
                                              MEM_RESERVE|MEM_COMMIT,
                                              PAGE_READWRITE);
    gameMemory.permanentStorage = win32State.gameMemoryBlock;
    gameMemory.transientStorage = (((u8 *)gameMemory.permanentStorage) +
                                   gameMemory.permanentStorageSize);
    gameMemory.renderOnGPU = renderOnGPU;

    InitRendererData Data;
    Data.Window = Window;
    Data.WindowWidth = Width;
    Data.WindowHeight = Height;
    Data.Memory = &gameMemory;
    initRenderer(&Data);

    LARGE_INTEGER Freq, C1, C2, C3;
    QueryPerformanceFrequency(&Freq);
    QueryPerformanceCounter(&C1);

    GameControllerInput OldInput = {};
    GameControllerInput NewInput = {};

    RAWINPUTDEVICE Rid[2];
    Rid[0].usUsagePage = 0x01;
    Rid[0].usUsage = 0x02;
    Rid[0].dwFlags = 0;
    Rid[0].hwndTarget = 0;
    Rid[1].usUsagePage = 0x01;
    Rid[1].usUsage = 0x06;
    Rid[1].dwFlags = 0;
    Rid[1].hwndTarget = 0;
    ASSERT(RegisterRawInputDevices(Rid, 2, sizeof(Rid[0])));

#if DEBUG_CONSOLE_ENABLED
    FILE* fp;
    AllocConsole();
    freopen_s(&fp, "CONOUT$", "w", stderr);
#endif

    GlobalRunning = true;
    while (GlobalRunning)
    {
        QueryPerformanceCounter(&C2);
        r32 DeltaTimeSec = (r32)((r64)(C2.QuadPart - C1.QuadPart) / Freq.QuadPart);
        C1 = C2;

        NewInput = {};
        for (u32 ButtonIdx = 0;
             ButtonIdx < ARRAY_COUNT(OldInput.buttons);
             ++ButtonIdx)
        {
            NewInput.buttons[ButtonIdx].endedDown = OldInput.buttons[ButtonIdx].endedDown;
        }

        Win32ProcessPendingMessages(&NewInput);

        POINT MouseP;
        GetCursorPos(&MouseP);
        ScreenToClient(Window, &MouseP);
        NewInput.mouseX = (r32)MouseP.x;
        NewInput.mouseY = (r32)MouseP.y;
        Win32ProcessKeyboardMessage(&NewInput.mouseButtons[0],
                                    GetKeyState(VK_LBUTTON) & (1 << 15));
        Win32ProcessKeyboardMessage(&NewInput.mouseButtons[1],
                                    GetKeyState(VK_MBUTTON) & (1 << 15));

#if DISPLAY_FRAME_TIME
        FPRINTF3(stderr, "%f\n", DeltaTimeSec);
#endif

        // get current size for window client area
        RECT Rect;
        GetClientRect(Window, &Rect);
        Width = Rect.right - Rect.left;
        Height = Rect.bottom - Rect.top;

        ThreadContext thread = {};
        SoundState soundState = {};

        updateGameAndRender(&thread, DeltaTimeSec, &NewInput, &soundState, &gameMemory, (r32)Width, (r32)Height);

        OldInput = NewInput;

        QueryPerformanceCounter(&C3);
        r32 TimeItTookToGenFrame = (r32)((r64)(C3.QuadPart - C2.QuadPart) / Freq.QuadPart);
        // NOTE(dima): if vsync is disabled, force 60 fps
        // TODO(dima): do not hardcode frametime, resolve it dynamically
        r32 frameTimeEpsilon = 0.001f;
        if (TimeItTookToGenFrame < 0.016f)
        {
            Sleep((u32)(1000*(0.016f - TimeItTookToGenFrame - frameTimeEpsilon)));
        }
    }

    ExitProcess(0);
}

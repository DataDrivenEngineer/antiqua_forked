#ifndef _ANTIQUA_PLATFORM_H_
#define _ANTIQUA_PLATFORM_H_

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !defined(COMPILER_MSVC)
#define COMPILER_MSVC 0
#endif

#if !COMPILER_MSVC && !COMPILER_LLVM
#if _MSC_VER
#undef COMPILER_MSVC
#define COMPILER_MSVC 1
#else
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#if COMPILER_LLVM

#define DIR_SEPARATOR "/"

#include <stddef.h>

typedef unsigned int b32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef size_t MemoryIndex;

typedef float r32;
typedef double r64;

#elif COMPILER_MSVC

#define DIR_SEPARATOR "\\"

#include <stdint.h>

typedef uint32_t b32;

typedef uint8_t u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;

typedef int8_t s8;
typedef int16_t s16;
typedef int32_t s32;
typedef int64_t s64;

typedef size_t MemoryIndex;

typedef float r32;
typedef double r64;
#endif

#define internal static
#define local_persist static
#define global_variable static

#if ANTIQUA_SLOW
#define ASSERT(expression) if (!(expression)) { *(u8 *) 0 = 0; }
#else
#define ASSERT(expression)
#endif

#if ANTIQUA_SLOW
#define FPRINTF2(arg1, arg2) fprintf((arg1), (arg2));
#define FPRINTF3(arg1, arg2, arg3) fprintf((arg1), (arg2), (arg3));
#else
#define FPRINTF2(arg1, arg2) 
#define FPRINTF3(arg1, arg2, arg3) 
#endif

#define INVALID_CODE_PATH ASSERT(!"InvalidCodePath")

#define PI32 3.14159265359f
#define RADIANS(value) ((value)/180.0f)*PI32
#define DEGREES(value) (value)*(180.0f/PI32)

#define ARRAY_COUNT(arr) sizeof(arr) / sizeof((arr)[0])
#define KB(Value) ((Value) * 1024LL)
#define MB(Value) (KB(Value) * 1024LL)
#define GB(Value) (MB(Value) * 1024LL)
#define MINIMUM(A, B) ((A < B) ? (A) : (B))
#define MAXIMUM(A, B) ((A > B) ? (A) : (B))

typedef struct
{
  MemoryIndex size;
  u8 *base;
  MemoryIndex used;
} MemoryArena;

typedef struct
{
  s32 placeholder;
} ThreadContext;

void initializeArena(MemoryArena *arena, MemoryIndex size, u8 *base)
{
  arena->size = size;
  arena->base = base;
  arena->used = 0;
}

#define PUSH_STRUCT(arena, Type) (Type *) pushSize_(arena, sizeof(Type))
#define PUSH_ARRAY(arena, count, Type) (Type *) pushSize_(arena, (count) * sizeof(Type))
#define PUSH_SIZE(arena, size, Type) (Type *) pushSize_(arena, (size))
void * pushSize_(MemoryArena *arena, MemoryIndex size)
{
  ASSERT((arena->used + size) <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;

  return result;
}

typedef struct
{
  b32 soundPlaying;
  u32 sampleRate;
  u32 toneHz;
  r32 frameOffset;
  r32 *frames;
  u32 needFrames;
  r32 tSine;
} SoundState;

typedef struct
{
  u32 halfTransitionCount;
  b32 endedDown;
} GameButtonState;

typedef struct
{
  // Index 0 = LMB, index 1 = RMB
  GameButtonState mouseButtons[2];
  r32 mouseX, mouseY, mouseZ;
  s32 scrollingDeltaX, scrollingDeltaY;

  b32 isAnalog;

  r32 stickAverageX;
  r32 stickAverageY;

  union
  {
    GameButtonState buttons[13];
    struct
    {
      GameButtonState up;
      GameButtonState down;
      GameButtonState left;
      GameButtonState right;
      GameButtonState actionUp;
      GameButtonState actionDown;
      GameButtonState actionLeft;
      GameButtonState actionRight;
      GameButtonState leftShoulder;
      GameButtonState rightShoulder;
      GameButtonState leftBumper;
      GameButtonState rightBumper;

      GameButtonState debugCmdC;

      // Add new buttons above
      GameButtonState terminator;
    };
  };
} GameControllerInput;

// Services that the platform provides to the game
#if ANTIQUA_INTERNAL
typedef struct
{
  u32 contentsSize;
  void *contents;
} debug_ReadFileResult;
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) b32 name(ThreadContext *thread, debug_ReadFileResult *outFile, const char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(Debug_PlatformReadEntireFile);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(ThreadContext *thread, debug_ReadFileResult *file)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(Debug_PlatformFreeFileMemory);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(ThreadContext *thread, const char *filename, u32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Debug_PlatformWriteEntireFile);
#endif

#define RESET_INPUT_STATE_BUTTONS(name) void name(ThreadContext *thread)
typedef RESET_INPUT_STATE_BUTTONS(ResetInputStateButtons);

#define LOCK_AUDIO_THREAD(name) void name(ThreadContext *thread)
typedef LOCK_AUDIO_THREAD(LockAudioThread);
#define UNLOCK_AUDIO_THREAD(name) void name(ThreadContext *thread)
typedef UNLOCK_AUDIO_THREAD(UnlockAudioThread);
#define WAIT_IF_AUDIO_BLOCKED(name) void name(ThreadContext *thread)
typedef WAIT_IF_AUDIO_BLOCKED(WaitIfAudioBlocked);

#define LOCK_INPUT_THREAD(name) void name(ThreadContext *thread)
typedef LOCK_INPUT_THREAD(LockInputThread);
#define UNLOCK_INPUT_THREAD(name) void name(ThreadContext *thread)
typedef UNLOCK_INPUT_THREAD(UnlockInputThread);
#define WAIT_IF_INPUT_BLOCKED(name) void name(ThreadContext *thread)
typedef WAIT_IF_INPUT_BLOCKED(WaitIfInputBlocked);

struct RenderGroup;
// NOTE(dima): data depends on the platform!
#define INIT_RENDERER(name) void name(void *data)
typedef INIT_RENDERER(InitRenderer);
#define RENDER_ON_GPU(name) void name(ThreadContext *thread, MemoryArena *arena, struct RenderGroup *renderGroup, s32 width, s32 height)
typedef RENDER_ON_GPU(RenderOnGPU);
#define RESIZE_WINDOW(name) void name(u32 width, u32 height, b32 minimized)

typedef struct
{
  b32 isInitialized;
  u64 permanentStorageSize;
  void *permanentStorage;
  u64 transientStorageSize;
  void *transientStorage;

  ResetInputStateButtons *resetInputStateButtons;

  LockAudioThread *lockAudioThread;
  UnlockAudioThread *unlockAudioThread;
  WaitIfAudioBlocked *waitIfAudioBlocked;
  LockInputThread *lockInputThread;
  UnlockInputThread *unlockInputThread;
  WaitIfInputBlocked *waitIfInputBlocked;

  RenderOnGPU *renderOnGPU;

#if ANTIQUA_INTERNAL
  Debug_PlatformReadEntireFile *debug_platformReadEntireFile;
  Debug_PlatformFreeFileMemory *debug_platformFreeFileMemory;
  Debug_PlatformWriteEntireFile *debug_platformWriteEntireFile;
#endif
} GameMemory;

#define UPDATE_GAME_AND_RENDER(name) void name(ThreadContext *thread, r32 deltaTimeSec, GameControllerInput *gcInput, SoundState *soundState, GameMemory *memory, r32 drawableWidthWithoutScaleFactor, r32 drawableHeightWithoutScaleFactor)
typedef UPDATE_GAME_AND_RENDER(UpdateGameAndRender);
#if XCODE_BUILD
MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender);
#endif
#define FILL_SOUND_BUFFER(name) void name(ThreadContext *thread, SoundState *soundState)
typedef FILL_SOUND_BUFFER(FillSoundBuffer);
#if XCODE_BUILD
MONExternC FILL_SOUND_BUFFER(fillSoundBuffer);
#endif

#endif

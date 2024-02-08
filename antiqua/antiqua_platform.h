#ifndef _ANTIQUA_PLATFORM_H_
#define _ANTIQUA_PLATFORM_H_

#include "types.h"

#if ANTIQUA_SLOW
#define ASSERT(expression) if (!(expression)) { *(u8 *) 0 = 0; }
#else
#define ASSERT(expression)
#endif

#define PI32 3.14159265359f
#define ARRAY_COUNT(arr) sizeof(arr) / sizeof((arr)[0])
#define KB(Value) ((Value) * 1024LL)
#define MB(Value) (KB(Value) * 1024LL)
#define GB(Value) (MB(Value) * 1024LL)

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
  u32 width;
  u32 height;
  u32 bytesPerPixel;
  u32 pitch;
  u32 sizeBytes;
  u8 *memory;
} GameOffscreenBuffer;

typedef struct
{
  u32 halfTransitionCount;
  b32 endedDown;
} GameButtonState;

typedef struct
{
  // Index 0 = LMB, index 1 = RMB
  GameButtonState mouseButtons[2];
  s32 mouseX, mouseY, mouseZ;

  b32 isAnalog;

  r32 stickAverageX;
  r32 stickAverageY;

  union
  {
    GameButtonState buttons[12];
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

      // Add new buttons above
      GameButtonState terminator;
    };
  };
} GameControllerInput;

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

#if ANTIQUA_INTERNAL
  Debug_PlatformReadEntireFile *debug_platformReadEntireFile;
  Debug_PlatformFreeFileMemory *debug_platformFreeFileMemory;
  Debug_PlatformWriteEntireFile *debug_platformWriteEntireFile;
#endif
} GameMemory;

#define UPDATE_GAME_AND_RENDER(name) void name(ThreadContext *thread, r32 deltaTimeSec, GameControllerInput *gcInput, SoundState *soundState, GameMemory *memory, GameOffscreenBuffer *buff)
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

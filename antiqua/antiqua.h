#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

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

struct SoundState
{
  u8 soundPlaying;
  u32 sampleRate;
  u32 toneHz;
  r32 frameOffset;
  r32 *frames;
  u32 needFrames;
  r32 tSine;
};

struct GameOffscreenBuffer
{
  u32 width;
  u32 height;
  u32 bytesPerPixel;
  u32 pitch;
  u32 sizeBytes;
  u8 *memory;
};

struct GameButtonState
{
  u32 halfTransitionCount;
  u8 endedDown;
};

struct GameControllerInput
{
  u8 isAnalog;

  r32 stickAverageX;
  r32 stickAverageY;

  union
  {
    struct GameButtonState buttons[12];
    struct
    {
      struct GameButtonState up;
      struct GameButtonState down;
      struct GameButtonState left;
      struct GameButtonState right;
      struct GameButtonState actionUp;
      struct GameButtonState actionDown;
      struct GameButtonState actionLeft;
      struct GameButtonState actionRight;
      struct GameButtonState leftShoulder;
      struct GameButtonState rightShoulder;
      struct GameButtonState leftBumper;
      struct GameButtonState rightBumper;

      // Add new buttons above
      struct GameButtonState terminator;
    };
  };
};

struct GameState
{
  u64 xOff;
  u64 yOff;

  s32 playerX;
  s32 playerY;

  r32 tJump;
};

// Services that the platform provides to the game
#if ANTIQUA_INTERNAL
struct debug_ReadFileResult
{
  u32 contentsSize;
  void *contents;
};
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) u8 name(struct debug_ReadFileResult *outFile, const char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(Debug_PlatformReadEntireFile);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(struct debug_ReadFileResult *file)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(Debug_PlatformFreeFileMemory);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) u8 name(const char *filename, u32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Debug_PlatformWriteEntireFile);
#endif

#define RESET_INPUT_STATE_BUTTONS(name) void name(void)
typedef RESET_INPUT_STATE_BUTTONS(ResetInputStateButtons);

#define LOCK_AUDIO_THREAD(name) void name(void)
typedef LOCK_AUDIO_THREAD(LockAudioThread);
#define UNLOCK_AUDIO_THREAD(name) void name(void)
typedef UNLOCK_AUDIO_THREAD(UnlockAudioThread);
#define WAIT_IF_AUDIO_BLOCKED(name) void name(void)
typedef WAIT_IF_AUDIO_BLOCKED(WaitIfAudioBlocked);

#define LOCK_INPUT_THREAD(name) void name(void)
typedef LOCK_INPUT_THREAD(LockInputThread);
#define UNLOCK_INPUT_THREAD(name) void name(void)
typedef UNLOCK_INPUT_THREAD(UnlockInputThread);
#define WAIT_IF_INPUT_BLOCKED(name) void name(void)
typedef WAIT_IF_INPUT_BLOCKED(WaitIfInputBlocked);

struct GameMemory
{
  u8 isInitialized;
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
};

#define UPDATE_GAME_AND_RENDER(name) void name(struct GameControllerInput *gcInput, struct SoundState *soundState, struct GameMemory *memory, struct GameOffscreenBuffer *buff)
typedef UPDATE_GAME_AND_RENDER(UpdateGameAndRender);
#if XCODE_BUILD
MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender);
#endif
#define FILL_SOUND_BUFFER(name) void name(struct SoundState *soundState)
typedef FILL_SOUND_BUFFER(FillSoundBuffer);
#if XCODE_BUILD
MONExternC FILL_SOUND_BUFFER(fillSoundBuffer);
#endif

#endif

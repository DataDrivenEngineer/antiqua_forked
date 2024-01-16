#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "types.h"
#include "antiqua_tile.h"
#include "antiqua_intrinsics.h"

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
  b32 soundPlaying;
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
  b32 endedDown;
};

struct GameControllerInput
{
  // Index 0 = LMB, index 1 = RMB
  struct GameButtonState mouseButtons[2];
  s32 mouseX, mouseY, mouseZ;

  b32 isAnalog;

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

typedef struct MemoryArena
{
  MemoryIndex size;
  u8 *base;
  MemoryIndex used;
} MemoryArena;

void initializeArena(MemoryArena *arena, MemoryIndex size, u8 *base)
{
  arena->size = size;
  arena->base = base;
  arena->used = 0;
}

#define PUSH_STRUCT(arena, Type) (Type *) pushSize_(arena, sizeof(Type))
#define PUSH_ARRAY(arena, count, Type) (Type *) pushSize_(arena, (count) * sizeof(Type))
void * pushSize_(MemoryArena *arena, MemoryIndex size)
{
  ASSERT((arena->used + size) <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;

  return result;
}

typedef struct World
{
  TileMap *tileMap;
} World;

struct GameState
{
  MemoryArena worldArena;
  World *world;

  TileMapPosition playerP;
};

struct ThreadContext
{
  s32 placeholder;
};

// Services that the platform provides to the game
#if ANTIQUA_INTERNAL
struct debug_ReadFileResult
{
  u32 contentsSize;
  void *contents;
};
#define DEBUG_PLATFORM_READ_ENTIRE_FILE(name) b32 name(struct ThreadContext *thread, struct debug_ReadFileResult *outFile, const char *filename)
typedef DEBUG_PLATFORM_READ_ENTIRE_FILE(Debug_PlatformReadEntireFile);
#define DEBUG_PLATFORM_FREE_FILE_MEMORY(name) void name(struct ThreadContext *thread, struct debug_ReadFileResult *file)
typedef DEBUG_PLATFORM_FREE_FILE_MEMORY(Debug_PlatformFreeFileMemory);
#define DEBUG_PLATFORM_WRITE_ENTIRE_FILE(name) b32 name(struct ThreadContext *thread, const char *filename, u32 memorySize, void *memory)
typedef DEBUG_PLATFORM_WRITE_ENTIRE_FILE(Debug_PlatformWriteEntireFile);
#endif

#define RESET_INPUT_STATE_BUTTONS(name) void name(struct ThreadContext *thread)
typedef RESET_INPUT_STATE_BUTTONS(ResetInputStateButtons);

#define LOCK_AUDIO_THREAD(name) void name(struct ThreadContext *thread)
typedef LOCK_AUDIO_THREAD(LockAudioThread);
#define UNLOCK_AUDIO_THREAD(name) void name(struct ThreadContext *thread)
typedef UNLOCK_AUDIO_THREAD(UnlockAudioThread);
#define WAIT_IF_AUDIO_BLOCKED(name) void name(struct ThreadContext *thread)
typedef WAIT_IF_AUDIO_BLOCKED(WaitIfAudioBlocked);

#define LOCK_INPUT_THREAD(name) void name(struct ThreadContext *thread)
typedef LOCK_INPUT_THREAD(LockInputThread);
#define UNLOCK_INPUT_THREAD(name) void name(struct ThreadContext *thread)
typedef UNLOCK_INPUT_THREAD(UnlockInputThread);
#define WAIT_IF_INPUT_BLOCKED(name) void name(struct ThreadContext *thread)
typedef WAIT_IF_INPUT_BLOCKED(WaitIfInputBlocked);

struct GameMemory
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
};

#define UPDATE_GAME_AND_RENDER(name) void name(struct ThreadContext *thread, r32 deltaTimeSec, struct GameControllerInput *gcInput, struct SoundState *soundState, struct GameMemory *memory, struct GameOffscreenBuffer *buff)
typedef UPDATE_GAME_AND_RENDER(UpdateGameAndRender);
#if XCODE_BUILD
MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender);
#endif
#define FILL_SOUND_BUFFER(name) void name(struct ThreadContext *thread, struct SoundState *soundState)
typedef FILL_SOUND_BUFFER(FillSoundBuffer);
#if XCODE_BUILD
MONExternC FILL_SOUND_BUFFER(fillSoundBuffer);
#endif

#endif

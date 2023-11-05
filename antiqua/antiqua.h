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

extern struct GameOffscreenBuffer framebuffer;

extern struct GameMemory memory;

extern u8 soundPlaying;
extern struct SoundState soundState;

extern struct GameControllerInput gcInput;

struct SoundState
{
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
    struct GameButtonState buttons[8];
    struct
    {
      struct GameButtonState up;
      struct GameButtonState down;
      struct GameButtonState left;
      struct GameButtonState right;
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
};

struct GameMemory
{
  u8 isInitialized;
  u64 permanentStorageSize;
  void *permanentStorage;
  u64 transientStorageSize;
  void *transientStorage;
};

MONExternC void updateGameAndRender(struct GameMemory *memory, struct GameOffscreenBuffer *buff);
MONExternC void fillSoundBuffer(struct SoundState *soundState);
MONExternC void resetInputStateAll(void);
MONExternC void resetInputStateButtons(void);

// Services that the platform provides to the game
#if ANTIQUA_INTERNAL
struct debug_ReadFileResult
{
  u32 contentsSize;
  void *contents;
};
MONExternC u8 debug_platformReadEntireFile(struct debug_ReadFileResult *outFile, const char *filename);
MONExternC void debug_platformFreeFileMemory(struct debug_ReadFileResult *file);
MONExternC u8 debug_platformWriteEntireFile(const char *filename, u32 memorySize, void *memory);
#endif

#endif

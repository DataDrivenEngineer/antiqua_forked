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

  r32 startX;
  r32 startY;

  r32 minX;
  r32 minY;

  r32 maxX;
  r32 maxY;

  r32 endX;
  r32 endY;

  union
  {
    struct GameButtonState Buttons[8];
    struct
    {
      struct GameButtonState Up;
      struct GameButtonState Down;
      struct GameButtonState Left;
      struct GameButtonState Right;
      struct GameButtonState LeftShoulder;
      struct GameButtonState RightShoulder;
      struct GameButtonState LeftBumper;
      struct GameButtonState RightBumper;
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

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

MONExternC void updateGameAndRender(struct GameMemory *memory, struct GameOffscreenBuffer *buff);
MONExternC void fillSoundBuffer(struct SoundState *soundState);

#endif

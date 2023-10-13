#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "types.h"
#include <pthread.h>

#define PI32 3.14159265359f

extern pthread_mutex_t mutex;

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

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

MONExternC void updateGameAndRender(struct GameOffscreenBuffer *buff);
MONExternC void fillSoundBuffer(struct SoundState *soundState);

#endif

#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "types.h"

#define PI32 3.14159265359f

struct SoundState
{
  r32 sampleRate;
  r32 toneHz;
  r32 volume;
  u32 runningFrameIndex;
  r32 frameOffset;
  s16 *buf;
  u32 bufCapacity;
};

struct GameOffscreenBuffer
{
  u32 width;
  u32 height;
  u8 *memory;
};

void updateGameAndRender(struct GameOffscreenBuffer *buf, u64 xOff, u64 yOff);
void fillSoundBuffer(struct SoundState *soundState);

#endif

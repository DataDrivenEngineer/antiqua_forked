#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "types.h"

#define PI32 3.14159265359f

struct SoundState
{
  u32 sampleRate;
  u32 toneHz;
  u32 runningSampleCount;
  r32 frameOffset;
  r32 *frames;
  u32 needFrames;
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

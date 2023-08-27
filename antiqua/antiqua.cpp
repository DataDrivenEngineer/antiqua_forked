#include "antiqua.h"

static void renderGradient(struct GameOffscreenBuffer *buf, int xOffset, int yOffset)
{
    int pitch = buf->width * 4;
    u8 *row = buf->memory;
    for (int y = 0; y < buf->height; y++)
    {
      u8 *pixel = row;
      for (int x = 0; x < buf->width; x++)
      {
	*pixel++ = 0;
	*pixel++ = y + yOffset;
	*pixel++ = x + xOffset;
	pixel++;
      }
      row += pitch;
    }
}

void updateGameAndRender(struct GameOffscreenBuffer *buf, u64 xOff, u64 yOff)
{
  renderGradient(buf, xOff, yOff);
}

void fillSoundBuffer(struct SoundState *soundState)
{
  // we're just filling the entire buffer here
  // In a real game we might only fill part of the buffer and set the mAudioDataBytes
  // accordingly.
  u32 framesToGen = soundState->needFrames;

  // calc the samples per up/down portion of each square wave (with 50% period)
  u32 framesPerCycle = soundState->sampleRate / soundState->toneHz;

  r32 *bufferPos = soundState->frames;
  r32 frameOffset = soundState->frameOffset;

  while (framesToGen) {
    // calc rounded frames to generate and accumulate fractional error
    u32 frames;
    u32 needFrames = (u32) (round(framesPerCycle - frameOffset));
    frameOffset -= framesPerCycle - needFrames;

    // we may be at the end of the buffer, if so, place offset at location in wave and clip
    if (needFrames > framesToGen) {
      frameOffset += framesToGen;
      frames = framesToGen;
    }
    else {
      frames = needFrames;
    }
    framesToGen -= frames;
    
    // simply put the samples in
    for (int x = 0; x < frames; ++x) {
      r32 t = 2.f * PI32 * (r32) soundState->runningSampleCount / framesPerCycle;
      r32 sineValue = sinf(t);
      r32 sample = sineValue;
      NSLog(@"sample = %f", sample);
      *bufferPos++ = sample;
      *bufferPos++ = sample;
      soundState->runningSampleCount++;
    }
  }

  soundState->frameOffset = frameOffset;
}

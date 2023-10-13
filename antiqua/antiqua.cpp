#include "antiqua.h"
#include "osx_audio.h"
#include "osx_input.h"
#include "types.h"

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

static void renderGradient(struct GameOffscreenBuffer *buf, u64 xOffset, u64 yOffset)
{
    int pitch = buf->width * 4;
    u8 *row = buf->memory;
    for (u32 y = 0; y < buf->height; y++)
    {
      u8 *pixel = row;
      for (u32 x = 0; x < buf->width; x++)
      {
	*pixel++ = 0;
	*pixel++ = y + yOffset;
	*pixel++ = x + xOffset;
	pixel++;
      }
      row += pitch;
    }
}

void updateGameAndRender(struct GameOffscreenBuffer *buff)
{
  static u64 xOff = 0;
  static u64 yOff = 0;
  if (!soundPlaying)
  {
    initAudio();
    soundPlaying = playAudio();
  }

  pthread_mutex_lock(&mutex);
  if (gcInput.isAnalog)
  {
    s16 normalized = gcInput.endX - 127;
//    fprintf(stderr, "%d\n", normalized);
    xOff += normalized >> 2;

    normalized = gcInput.endY - 127;
    s32 toneHzModifier = (s32) (256.f * (normalized / 255.f));
    soundState.toneHz = 512 + toneHzModifier;
    yOff += normalized >> 2;
  }
  pthread_mutex_unlock(&mutex);

  renderGradient(buff, xOff, yOff);
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
    for (u32 x = 0; x < frames; ++x) {
      r32 sineValue = sinf(soundState->tSine);
      r32 sample = sineValue;
      *bufferPos++ = sample;
      *bufferPos++ = sample;
      soundState->tSine += 2.f * PI32 * 1.f / framesPerCycle;
    }
  }

  soundState->frameOffset = frameOffset;
}

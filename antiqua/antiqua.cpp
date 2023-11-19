#include <cmath>

#include "antiqua.h"
#include "types.h"

static void renderGradient(struct GameOffscreenBuffer *buf, u64 xOffset, u64 yOffset)
{
    u8 *row = buf->memory + buf->height * buf->pitch - buf->pitch;
    for (u32 y = 0; y < buf->height; y++)
    {
      u8 *pixel = row;
      for (u32 x = 0; x < buf->width; x++)
      {
	*pixel++ = 1;
	*pixel++ = y + yOffset;
	*pixel++ = x + xOffset;
	pixel++;
      }
      row -= buf->pitch;
    }
}

static void renderPlayer(struct GameOffscreenBuffer *buf, s32 playerX, s32 playerY)
{
  s32 top = playerY;
  s32 bottom = playerY + 10;
  u32 color = 0xFFFFFFFF;
  for (s32 x = playerX; x < playerX + 10; x++)
  {
    u8 *row = buf->memory + buf->height * buf->pitch - buf->pitch;
    u8 *pixel = row + x * buf->bytesPerPixel - top * buf->pitch;
    for (s32 y = bottom; y > top; y--)
    {
      if (pixel >= buf->memory && pixel <= buf->memory + buf->sizeBytes - 4)
      {
	*(u32 *)pixel = color;
	pixel -= buf->pitch;
      }
    }
  }
}

#if !XCODE_BUILD
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
  ASSERT(&gcInput->terminator - &gcInput->buttons[0] == ARRAY_COUNT(gcInput->buttons));
  ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
  GameState *gameState = (GameState *) memory->permanentStorage;
  if (!memory->isInitialized)
  {
    // do initialization here as needed
    const char *filename = __FILE__;
    struct debug_ReadFileResult file;
    if (memory->debug_platformReadEntireFile(thread, &file, filename))
    {
      memory->debug_platformWriteEntireFile(thread, "data/test.out", file.contentsSize, file.contents);
      memory->debug_platformFreeFileMemory(thread, &file);
    }

    gameState->playerX = 100;
    gameState->playerY = 100;

    memory->isInitialized = 1;
  }

  if (gcInput->isAnalog)
  {
    s16 normalizedX = gcInput->stickAverageX - 127;
//    fprintf(stderr, "%d\n", normalized);
    gameState->xOff += normalizedX >> 2;

    s16 normalizedY = gcInput->stickAverageY - 127;
    s32 toneHzModifier = (s32) (256.f * (normalizedY / 255.f));
    memory->waitIfAudioBlocked(thread);
    memory->lockAudioThread(thread);
    soundState->toneHz = 512 + toneHzModifier;
    memory->unlockAudioThread(thread);
    gameState->yOff += normalizedY >> 2;

    gameState->playerX += ((s32) (normalizedX) >> 3);
    gameState->playerY += ((s32) (normalizedY) >> 3);
    if (gameState->tJump > 0)
    {
      gameState->playerY -= ((s32) (normalizedY) >> 5) - 10.f * sinf(2.f * PI32 * gameState->tJump);
    }
    if (gcInput->actionUp.endedDown)
    {
      gameState->tJump = 1.f;
    }
  }
  else
  {
    if (gcInput->right.endedDown)
    {
      gameState->xOff += 2;
    }
    if (gcInput->left.endedDown)
    {
      gameState->xOff -= 2;
    }
    if (gcInput->up.endedDown)
    {
      gameState->yOff -= 2;
    }
    if (gcInput->down.endedDown)
    {
      gameState->yOff += 2;
    }
  }

  gameState->tJump -= 0.033f;

  memory->waitIfInputBlocked(thread);
  memory->lockInputThread(thread);
  memory->resetInputStateButtons(thread);
  memory->unlockInputThread(thread);

  renderGradient(buff, gameState->xOff, gameState->yOff);
  renderPlayer(buff, gameState->playerX, gameState->playerY);

  for (u8 buttonIndex = 0; buttonIndex < ARRAY_COUNT(gcInput->mouseButtons); buttonIndex++)
  {
    if (gcInput->mouseButtons[buttonIndex].endedDown)
    {
      renderPlayer(buff, 10 + 20 * buttonIndex, 10);
    }
  }

  renderPlayer(buff, gcInput->mouseX, gcInput->mouseY);
}

#if !XCODE_BUILD
EXPORT MONExternC FILL_SOUND_BUFFER(fillSoundBuffer)
#else
FILL_SOUND_BUFFER(fillSoundBuffer)
#endif
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
#if 1
      r32 sineValue = sinf(soundState->tSine);
#else
      r32 sineValue = 0;
#endif
      r32 sample = sineValue;
      *bufferPos++ = sample;
      *bufferPos++ = sample;
      soundState->tSine += 2.f * PI32 * 1.f / framesPerCycle;
      if (soundState->tSine > 2.f * PI32)
      {
	soundState->tSine -= 2.f * PI32;
      }
    }
  }

  soundState->frameOffset = frameOffset;
}

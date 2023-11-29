#include <cmath>

#include "antiqua.h"
#include "types.h"
#include "stdio.h"

static s32 roundReal32ToInt32(r32 v)
{
  s32 result = (s32) (v + 0.5f);
  return result;
}

static u32 roundReal32ToUInt32(r32 v)
{
  u32 result = (u32) (v + 0.5f);
  return result;
}

static void drawRectangle(struct GameOffscreenBuffer *buf, r32 realMinX, r32 realMinY, r32 realMaxX, r32 realMaxY, r32 r, r32 g, r32 b)
{
  if (realMaxX < 0 || realMaxY < 0)
  {
    return;
  }

  s32 minX = roundReal32ToInt32(realMinX);
  s32 minY = roundReal32ToInt32(realMinY);
  s32 maxX = roundReal32ToInt32(realMaxX);
  s32 maxY = roundReal32ToInt32(realMaxY);

  if (minX < 0)
  {
    minX = 0;
  }

  if (minY < 0)
  {
    minY = 0;
  }

  if (maxX > buf->width)
  {
    maxX = buf->width;
  }

  if (maxY > buf->height)
  {
    maxY = buf->height;
  }

  // NOTE(dima): byte ordering here is in BGRA format
  u32 color = (roundReal32ToUInt32(b * 255.f) << 16) | (roundReal32ToUInt32(g * 255.f) << 8) | roundReal32ToUInt32(r * 255.f);

  u8 *row = (u8 *) buf->memory + minX * buf->bytesPerPixel + minY * buf->pitch;
  for (s32 y = minY; y < maxY; ++y)
  {
    u32 *pixel = (u32 *) row;
    for (s32 x = minX; x < maxX; ++x)
    {
      *pixel++ = color;
    }

    row += buf->pitch;
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
    memory->isInitialized = 1;
  }

  if (gcInput->isAnalog)
  {
  }
  else
  {
    r32 dPlayerX = 0.f;
    r32 dPlayerY = 0.f;
    if (gcInput->up.endedDown)
    {
      dPlayerY = -1.f;
    }
    if (gcInput->down.endedDown)
    {
      dPlayerY = 1.f;
    }
    if (gcInput->left.endedDown)
    {
      dPlayerX = -1.f;
    }
    if (gcInput->right.endedDown)
    {
      dPlayerX = 1.f;
    }
    dPlayerX *= 128.f;
    dPlayerY *= 128.f;

    gameState->playerX += deltaTimeSec * dPlayerX;
    gameState->playerY += deltaTimeSec * dPlayerY;
  }

  u32 tileMap[9][17] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
    {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1},
    {0, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0},
    {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1},
    {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1}
  };

  r32 upperLeftX = -30;
  r32 upperLeftY = 0;
  r32 tileWidth = 60;
  r32 tileHeight = 60;

  for (s32 row = 0; row < 9; ++row)
  {
    for (s32 column = 0; column < 17; column++)
    {
      u32 tileID = tileMap[row][column];
      r32 gray = 0.5f;
      if (tileID == 1)
      {
	gray = 1.f;
      }

      r32 minX = upperLeftX + ((r32) column) * tileWidth;
      r32 minY = upperLeftY + ((r32) row) * tileHeight;
      r32 maxX = minX + tileWidth;
      r32 maxY = minY + tileHeight;
      drawRectangle(buff, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  r32 playerR = 1.f;
  r32 playerG = 1.f;
  r32 playerB = 0.f;
  r32 playerWidth = 0.75f * tileWidth;
  r32 playerHeight = tileHeight;
  r32 playerLeft = gameState->playerX - 0.5f * playerWidth;
  r32 playerTop = gameState->playerY - playerHeight;
  drawRectangle(buff, playerLeft, playerTop, playerLeft + playerWidth, playerTop + playerHeight, playerR, playerG, playerB);

  memory->waitIfInputBlocked(thread);
  memory->lockInputThread(thread);
  memory->resetInputStateButtons(thread);
  memory->unlockInputThread(thread);
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
#if 0
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

#if 0
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

#endif


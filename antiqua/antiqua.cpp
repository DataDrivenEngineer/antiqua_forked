#include <cmath>

#include "antiqua.h"
#include "types.h"
#include "stdio.h"

static inline s32 roundReal32ToInt32(r32 v)
{
  s32 result = (s32) (v + 0.5f);
  return result;
}

static inline u32 roundReal32ToUInt32(r32 v)
{
  u32 result = (u32) (v + 0.5f);
  return result;
}

static inline s32 floorReal32ToInt32(r32 v)
{
  s32 result = (s32) floorf(v);
  return result;
}

static inline s32 truncateReal32ToInt32(r32 v)
{
  s32 result = (s32) v;
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

inline TileMap * getTileMap(World *world, s32 tileMapX, s32 tileMapY)
{
  TileMap *tileMap = 0;

  if (tileMapX >= 0 && tileMapX < world->tileMapCountX &&
    tileMapY >= 0 && tileMapY < world->tileMapCountY)
  {
    tileMap = &world->tileMaps[tileMapY * world->tileMapCountX + tileMapX];
  }

  return tileMap;
}

inline u32 getTileValueUnchecked(World *world, TileMap *tileMap, s32 tileX, s32 tileY)
{
  ASSERT(tileMap);
  ASSERT(tileX >= 0 && tileX < world->countX &&
    tileY >= 0 && tileY < world->countY);

  u32 tileMapValue = tileMap->tiles[tileY * world->countX + tileX];
  return tileMapValue;
}

static b32 isTileMapPointEmpty(World *world, TileMap *tileMap, r32 testTileX, r32 testTileY)
{
  b32 empty = 0;

  if (tileMap)
  {
    if (testTileX >= 0 && testTileX < world->countX && testTileY >= 0 && testTileY < world->countY)
    {
      u32 tileMapValue = getTileValueUnchecked(world, tileMap, testTileX, testTileY);
      empty = tileMapValue == 0;
    }
  }

  return empty;
}

static inline void recanonicalizeCoord(World *world, s32 tileCount, s32 *tileMap, s32 *tile, r32 *tileRel)
{
  s32 offset = floorReal32ToInt32(*tileRel / world->tileSideInMeters);
  *tile += offset;
  *tileRel -= offset * world->tileSideInMeters;

  ASSERT(*tileRel >= 0);
  ASSERT(*tileRel < world->tileSideInMeters);

  if (*tile < 0)
  {
    *tile = tileCount + *tile;
    --*tileMap;
  }

  if (*tile >= tileCount)
  {
    *tile = *tile - tileCount;
    ++*tileMap;
  }
}

static inline CanonicalPosition recanonicalizePosition(World* world, CanonicalPosition pos)
{
  CanonicalPosition result = pos;

  recanonicalizeCoord(world, world->countX, &result.tileMapX, &result.tileX, &result.tileRelX);
  recanonicalizeCoord(world, world->countY, &result.tileMapY, &result.tileY, &result.tileRelY);

  return result;
}

static b32 isWorldPointEmpty(World *world, CanonicalPosition canPos)
{
  b32 empty = 0;

  TileMap *tileMap = getTileMap(world, canPos.tileMapX, canPos.tileMapY);
  empty = isTileMapPointEmpty(world, tileMap, canPos.tileX, canPos.tileY);

  return empty;
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
    gameState->playerP.tileMapX = 0;
    gameState->playerP.tileMapY = 0;
    gameState->playerP.tileX = 3;
    gameState->playerP.tileY = 3;
    gameState->playerP.tileRelX = 0.5f;
    gameState->playerP.tileRelY = 0.5f;

    memory->isInitialized = 1;
  }

#define TILE_MAP_COUNT_X 17
#define TILE_MAP_COUNT_Y 9

  u32 tiles00[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1},
    {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  0},
    {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1},
    {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1}
  };

  u32 tiles01[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1}
  };

  u32 tiles10[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1},
    {1, 1, 0, 0,  0, 1, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 1, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 1, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1},
    {0, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 1, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 0, 0, 0,  0, 1, 0, 0,  1, 0, 0, 0,  1, 0, 0, 0,  1},
    {1, 1, 1, 1,  1, 0, 0, 0,  0, 0, 0, 0,  0, 1, 0, 0,  1},
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1}
  };

  u32 tiles11[TILE_MAP_COUNT_Y][TILE_MAP_COUNT_X] =
  {
    {1, 1, 1, 1,  1, 1, 1, 1,  0, 1, 1, 1,  1, 1, 1, 1,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  0, 0, 0, 0,  1},
    {1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1, 1, 1, 1,  1}
  };

  TileMap tileMaps[2][2];

  tileMaps[0][0].tiles = (u32 *) tiles00;
  tileMaps[0][1].tiles = (u32 *) tiles10;
  tileMaps[1][0].tiles = (u32 *) tiles01;
  tileMaps[1][1].tiles = (u32 *) tiles11;

  World world;

  world.tileSideInMeters = 1.4f;
  world.tileSideInPixels = 60;
  world.metersToPixels = (r32) world.tileSideInPixels / world.tileSideInMeters;

  world.tileMapCountX = 2;
  world.tileMapCountY = 2;
  world.countX = TILE_MAP_COUNT_X;
  world.countY = TILE_MAP_COUNT_Y;
  world.upperLeftX = -world.tileSideInPixels / 2;
  world.upperLeftY = 0;

  world.tileMaps = (TileMap *) tileMaps;

  r32 playerHeight = 1.4f;
  r32 playerWidth = 0.75f * playerHeight;

  TileMap *tileMap = getTileMap(&world, gameState->playerP.tileMapX, gameState->playerP.tileMapY);
  ASSERT(tileMap);

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
    dPlayerX *= 2.f;
    dPlayerY *= 2.f;

    CanonicalPosition newPlayerP = gameState->playerP;
    newPlayerP.tileRelX += deltaTimeSec * dPlayerX;
    newPlayerP.tileRelY += deltaTimeSec * dPlayerY;
    newPlayerP = recanonicalizePosition(&world, newPlayerP);

    CanonicalPosition playerLeft = newPlayerP;
    playerLeft.tileRelX -= 0.5f * playerWidth;
    playerLeft = recanonicalizePosition(&world, playerLeft);

    CanonicalPosition playerRight = newPlayerP;
    playerRight.tileRelX += 0.5f * playerWidth;
    playerRight = recanonicalizePosition(&world, playerRight);

    if (isWorldPointEmpty(&world, newPlayerP)
      && isWorldPointEmpty(&world, playerLeft)
      && isWorldPointEmpty(&world, playerRight))
    {
      gameState->playerP = newPlayerP;
    }
  }

  for (s32 row = 0; row < 9; ++row)
  {
    for (s32 column = 0; column < 17; column++)
    {
      u32 tileID = getTileValueUnchecked(&world, tileMap, column, row);
      r32 gray = 0.5f;
      if (tileID == 1)
      {
	gray = 1.f;
      }

      if (column == gameState->playerP.tileX && row == gameState->playerP.tileY)
      {
	gray = 0.0f;
      }

      r32 minX = world.upperLeftX + ((r32) column) * world.tileSideInPixels;
      r32 minY = world.upperLeftY + ((r32) row) * world.tileSideInPixels;
      r32 maxX = minX + world.tileSideInPixels;
      r32 maxY = minY + world.tileSideInPixels;
      drawRectangle(buff, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  r32 playerR = 1.f;
  r32 playerG = 1.f;
  r32 playerB = 0.f;
  r32 playerLeft = world.upperLeftX + world.tileSideInPixels * gameState->playerP.tileX + world.metersToPixels * gameState->playerP.tileRelX - 0.5f * world.metersToPixels * playerWidth;
  r32 playerTop = world.upperLeftY + world.tileSideInPixels * gameState->playerP.tileY + world.metersToPixels * gameState->playerP.tileRelY - world.metersToPixels * playerHeight;
  drawRectangle(buff, playerLeft, playerTop, playerLeft + world.metersToPixels * playerWidth, playerTop + world.metersToPixels * playerHeight, playerR, playerG, playerB);

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


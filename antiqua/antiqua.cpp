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

#include <cmath>
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

static inline CanonicalPosition getCanonicalPosition(World* world, RawPosition pos)
{
  CanonicalPosition result;

  result.tileMapX = pos.tileMapX;
  result.tileMapY = pos.tileMapY;

  r32 x = pos.x - world->upperLeftX;
  r32 y = pos.y - world->upperLeftY;
  result.tileX = floorReal32ToInt32(x / world->tileWidth);
  result.tileY = floorReal32ToInt32(y / world->tileHeight);

  result.x = x - result.tileX * world->tileWidth;
  result.y = y - result.tileY * world->tileHeight;

  ASSERT(result.x >= 0);
  ASSERT(result.y >= 0);
  ASSERT(result.x < world->tileWidth);
  ASSERT(result.y < world->tileHeight);

  if (result.tileX < 0)
  {
    result.tileX = world->countX + result.tileX;
    --result.tileMapX;
  }

  if (result.tileY < 0)
  {
    result.tileY = world->countY + result.tileY;
    --result.tileMapY;
  }

  if (result.tileX >= world->countX)
  {
    result.tileX = result.tileX - world->countX;
    ++result.tileMapX;
  }

  if (result.tileY >= world->countY)
  {
    result.tileY = result.tileY - world->countY;
    ++result.tileMapY;
  }

  return result;
}

static b32 isWorldPointEmpty(World *world, RawPosition testPos)
{
  b32 empty = 0;

  CanonicalPosition canPos = getCanonicalPosition(world, testPos);
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
    gameState->playerTileMapX = 0;
    gameState->playerTileMapY = 0;
    gameState->playerX = 150;
    gameState->playerY = 150;
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
  world.tileMapCountX = 2;
  world.tileMapCountY = 2;
  world.countX = TILE_MAP_COUNT_X;
  world.countY = TILE_MAP_COUNT_Y;
  world.upperLeftX = -30;
  world.upperLeftY = 0;
  world.tileWidth = 60;
  world.tileHeight = 60;

  world.tileMaps = (TileMap *) tileMaps;

  r32 playerWidth = 0.75f * world.tileWidth;
  r32 playerHeight = world.tileHeight;

  TileMap *tileMap = getTileMap(&world, gameState->playerTileMapX, gameState->playerTileMapY);
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
    dPlayerX *= 128.f;
    dPlayerY *= 128.f;

    r32 newPlayerX = gameState->playerX + deltaTimeSec * dPlayerX;
    r32 newPlayerY = gameState->playerY + deltaTimeSec * dPlayerY;

    RawPosition playerPos = {gameState->playerTileMapX, gameState->playerTileMapY, newPlayerX, newPlayerY};
    RawPosition playerLeft = playerPos;
    playerLeft.x -= 0.5f * playerWidth;
    RawPosition playerRight = playerPos;
    playerRight.x += 0.5f * playerWidth;

    if (isWorldPointEmpty(&world, playerPos)
      && isWorldPointEmpty(&world, playerLeft)
      && isWorldPointEmpty(&world, playerRight))
    {
      CanonicalPosition canPos = getCanonicalPosition(&world, playerPos);

      gameState->playerTileMapX = canPos.tileMapX;
      gameState->playerTileMapY = canPos.tileMapY;
      gameState->playerX = world.upperLeftX + world.tileWidth * canPos.tileX + canPos.x;
      gameState->playerY = world.upperLeftY + world.tileHeight * canPos.tileY + canPos.y;
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

      r32 minX = world.upperLeftX + ((r32) column) * world.tileWidth;
      r32 minY = world.upperLeftY + ((r32) row) * world.tileHeight;
      r32 maxX = minX + world.tileWidth;
      r32 maxY = minY + world.tileHeight;
      drawRectangle(buff, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  r32 playerR = 1.f;
  r32 playerG = 1.f;
  r32 playerB = 0.f;
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


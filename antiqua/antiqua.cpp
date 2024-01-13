#include "antiqua.h"
#include "types.h"
#include "stdio.h"
#include "antiqua_tile.h"

#include "antiqua_tile.cpp"

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

static void initializeArena(MemoryArena *arena, MemoryIndex size, u8 *base)
{
  arena->size = size;
  arena->base = base;
  arena->used = 0;
}

#define PUSH_STRUCT(arena, Type) (Type *) pushSize_(arena, sizeof(Type))
#define PUSH_ARRAY(arena, count, Type) (Type *) pushSize_(arena, (count) * sizeof(Type))
void * pushSize_(MemoryArena *arena, MemoryIndex size)
{
  ASSERT((arena->used + size) <= arena->size);
  void *result = arena->base + arena->used;
  arena->used += size;

  return result;
}

#if !XCODE_BUILD
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
  ASSERT(&gcInput->terminator - &gcInput->buttons[0] == ARRAY_COUNT(gcInput->buttons));
  ASSERT(sizeof(GameState) <= memory->permanentStorageSize);

  r32 playerHeight = 1.4f;
  r32 playerWidth = 0.75f * playerHeight;

  GameState *gameState = (GameState *) memory->permanentStorage;
  if (!memory->isInitialized)
  {
    // do initialization here as needed
    gameState->playerP.absTileX = 1;
    gameState->playerP.absTileY = 3;
    gameState->playerP.tileRelX = 5.0f;
    gameState->playerP.tileRelY = 5.0f;
    initializeArena(&gameState->worldArena, memory->permanentStorageSize - sizeof(GameState), (u8 *) memory->permanentStorage + sizeof(GameState));

    gameState->world = PUSH_STRUCT(&gameState->worldArena, World);
    World *world = gameState->world;
    world->tileMap = PUSH_STRUCT(&gameState->worldArena, TileMap);

    TileMap *tileMap = world->tileMap;

    // NOTE(dima): this is set to using 256x256 tile chunks
    tileMap->chunkShift = 4;
    tileMap->chunkMask = (1 << tileMap->chunkShift);
    tileMap->chunkMask -= 1;
    tileMap->chunkDim = 1 << tileMap->chunkShift;

    tileMap->tileSideInMeters = 1.4f;
    tileMap->tileSideInPixels = 60;
    tileMap->metersToPixels = (r32) tileMap->tileSideInPixels / tileMap->tileSideInMeters;

    tileMap->tileChunkCountX = 128;
    tileMap->tileChunkCountY = 128;

    tileMap->tileChunks = PUSH_ARRAY(&gameState->worldArena, tileMap->tileChunkCountX * tileMap->tileChunkCountY, TileChunk);

    for (u32 y = 0; y < tileMap->tileChunkCountY; y++)
    {
      for (u32 x = 0; x < tileMap->tileChunkCountX; x++)
      {
        tileMap->tileChunks[y * tileMap->tileChunkCountX + x].tiles = PUSH_ARRAY(&gameState->worldArena, tileMap->chunkDim * tileMap->chunkDim, u32);
      }
    }

    r32 lowerLeftX = -tileMap->tileSideInPixels / 2;
    r32 lowerLeftY = (r32) buff->height;

    u32 tilesPerWidth = 17;
    u32 tilesPerHeight = 9;
    for (u32 screenY = 0; screenY < 32; screenY++)
    {
      for (u32 screenX = 0; screenX < 32; screenX++)
      {
        for (u32 tileY = 0; tileY < tilesPerHeight; tileY++)
        {
          for (u32 tileX = 0; tileX < tilesPerWidth; tileX++)
          {
            u32 absTileX = screenX * tilesPerWidth + tileX;
            u32 absTileY = screenY * tilesPerHeight + tileY;

            setTileValue(&gameState->worldArena, world->tileMap, absTileX, absTileY, tileX == tileY && tileY % 2 ? 1 : 0);
          }
        }
      }
    }

    memory->isInitialized = 1;
  }

  World *world = gameState->world;
  TileMap *tileMap = world->tileMap;

  if (gcInput->isAnalog)
  {
  }
  else
  {
    r32 dPlayerX = 0.f;
    r32 dPlayerY = 0.f;
    if (gcInput->up.endedDown)
    {
      dPlayerY = 1.f;
    }
    if (gcInput->down.endedDown)
    {
      dPlayerY = -1.f;
    }
    if (gcInput->left.endedDown)
    {
      dPlayerX = -1.f;
    }
    if (gcInput->right.endedDown)
    {
      dPlayerX = 1.f;
    }

    r32 playerSpeed = 2.f;

    if (gcInput->actionUp.endedDown)
    {
      playerSpeed = 10.f;
    }

    dPlayerX *= playerSpeed;
    dPlayerY *= playerSpeed;

    TileMapPosition newPlayerP = gameState->playerP;
    newPlayerP.tileRelX += deltaTimeSec * dPlayerX;
    newPlayerP.tileRelY += deltaTimeSec * dPlayerY;
    newPlayerP = recanonicalizePosition(tileMap, newPlayerP);

    TileMapPosition playerLeft = newPlayerP;
    playerLeft.tileRelX -= 0.5f * playerWidth;
    playerLeft = recanonicalizePosition(tileMap, playerLeft);

    TileMapPosition playerRight = newPlayerP;
    playerRight.tileRelX += 0.5f * playerWidth;
    playerRight = recanonicalizePosition(tileMap, playerRight);

    if (isTileMapPointEmpty(tileMap, newPlayerP)
      && isTileMapPointEmpty(tileMap, playerLeft)
      && isTileMapPointEmpty(tileMap, playerRight))
    {
      gameState->playerP = newPlayerP;
    }
  }

  r32 screenCenterX = 0.5f * (r32) buff->width;
  r32 screenCenterY = 0.5f * (r32) buff->height;

  for (s32 relRow = -10; relRow < 10; ++relRow)
  {
    for (s32 relColumn = -20; relColumn < 20; relColumn++)
    {
      u32 column = gameState->playerP.absTileX + relColumn;
      u32 row = gameState->playerP.absTileY + relRow;
      u32 tileID = getTileValue(tileMap, column, row);
      r32 gray = 0.5f;
      if (tileID == 1)
      {
	gray = 1.f;
      }

      if (column == gameState->playerP.absTileX && row == gameState->playerP.absTileY)
      {
	gray = 0.0f;
      }

      r32 cenX = screenCenterX - tileMap->metersToPixels * gameState->playerP.tileRelX + ((r32) relColumn) * tileMap->tileSideInPixels;
      r32 cenY = screenCenterY + tileMap->metersToPixels * gameState->playerP.tileRelY - ((r32) relRow) * tileMap->tileSideInPixels;
      r32 minX = cenX - 0.5f * tileMap->tileSideInPixels;
      r32 minY = cenY - 0.5f * tileMap->tileSideInPixels;
      r32 maxX = cenX + 0.5f * tileMap->tileSideInPixels;
      r32 maxY = cenY + 0.5f * tileMap->tileSideInPixels;
      drawRectangle(buff, minX, minY, maxX, maxY, gray, gray, gray);
    }
  }

  r32 playerR = 1.f;
  r32 playerG = 1.f;
  r32 playerB = 0.f;
  r32 playerLeft = screenCenterX - 0.5f * tileMap->metersToPixels * playerWidth;
  r32 playerTop = screenCenterY - tileMap->metersToPixels * playerHeight;
  drawRectangle(buff, playerLeft, playerTop, playerLeft + tileMap->metersToPixels * playerWidth, playerTop + tileMap->metersToPixels * playerHeight, playerR, playerG, playerB);

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


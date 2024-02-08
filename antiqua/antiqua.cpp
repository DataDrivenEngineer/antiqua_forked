#include "antiqua.h"
#include "types.h"
#include "stdio.h"
#include "antiqua_random.h"
#include "antiqua_intrinsics.h"

#include "antiqua_tile.cpp"

static void drawRectangle(GameOffscreenBuffer *buf, V2 vMin, V2 vMax, r32 r, r32 g, r32 b)
{
  if (vMax.x < 0 || vMax.y < 0)
  {
    return;
  }

  s32 minX = roundReal32ToInt32(vMin.x);
  s32 minY = roundReal32ToInt32(vMin.y);
  s32 maxX = roundReal32ToInt32(vMax.x);
  s32 maxY = roundReal32ToInt32(vMax.y);

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

  // NOTE(dima): byte ordering here is in RGB format
  u32 color = (roundReal32ToUInt32(r * 255.f) << 16) | (roundReal32ToUInt32(g * 255.f) << 8) | (roundReal32ToUInt32(b * 255.f) << 0);

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

static void drawBitmap(GameOffscreenBuffer *buf, LoadedBitmap *bitmap, r32 realX, r32 realY, s32 alignX = 0, s32 alignY = 0)
{
  realX -= (r32) alignX;
  realY -= (r32) alignY;

  s32 minX = roundReal32ToInt32(realX);
  s32 minY = roundReal32ToInt32(realY);
  s32 maxX = roundReal32ToInt32(realX + (r32) bitmap->width);
  s32 maxY = roundReal32ToInt32(realY + (r32) bitmap->height);

  s32 sourceOffsetX = 0;
  if (minX < 0)
  {
    sourceOffsetX = -minX;
    minX = 0;
  }

  s32 sourceOffsetY = 0;
  if (minY < 0)
  {
    sourceOffsetY = -minY;
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

  u32 *sourceRow = bitmap->pixels + bitmap->width * (bitmap->height - 1);
  sourceRow += -sourceOffsetY * bitmap->width + sourceOffsetX;
  u8 *destRow = (u8 *) buf->memory + minX * buf->bytesPerPixel + minY * buf->pitch;
  for (s32 y = minY; y < maxY; ++y)
  {
    u32 *dest = (u32 *) destRow;
    u32 *source = sourceRow;
    for (s32 x = minX; x < maxX; ++x)
    {
      r32 a = (r32) ((*source >> 24) & 0xFF) / 255.f;
      r32 sr = (r32) ((*source >> 16) & 0xFF);
      r32 sg = (r32) ((*source >> 8) & 0xFF);
      r32 sb = (r32) ((*source >> 0) & 0xFF);

      r32 dr = (r32) ((*dest >> 16) & 0xFF);
      r32 dg = (r32) ((*dest >> 8) & 0xFF);
      r32 db = (r32) ((*dest >> 0) & 0xFF);

      r32 r = (1.f - a) * dr + a * sr;
      r32 g = (1.f - a) * dg + a * sg;
      r32 b = (1.f - a) * db + a * sb;

      *dest = ((u32) (r + 0.5f) << 16) |
              ((u32) (g + 0.5f) << 8) |
              ((u32) (b + 0.5f) << 0);

      ++dest;
      ++source;
    }

    destRow += buf->pitch;
    sourceRow -= bitmap->width;
  }
}

typedef struct
// NOTE(dima): This is to ensure that struct elements are byte-aligned in memory
// This will only work for clang compiler!
__attribute__((packed))
{
  u16 fileType;
  u32 fileSize;
  u16 reserved1;
  u16 reserved2;
  u32 bitmapOffset;
  u32 size;
  s32 width;
  s32 height;
  u16 planes;
  u16 bitsPerPixel;
  u32 compression;
  u32 sizeOfBitmap;
  s32 horzResolution;
  s32 vertResolution;
  u32 colorsUsed;
  u32 colorsImportant;

  u32 redMask;
  u32 greenMask;
  u32 blueMask;
} BitmapHeader;

static LoadedBitmap Debug_LoadBMP(ThreadContext *thread, Debug_PlatformReadEntireFile *readEntireFile, const char *filename)
{
  LoadedBitmap result = {0};

  debug_ReadFileResult readResult = {0};
  readEntireFile(thread, &readResult, filename);
  if (readResult.contentsSize != 0)
  {
    BitmapHeader *header = (BitmapHeader *) readResult.contents;
    u32 *pixels = (u32 *) ((u8 *) readResult.contents + header->bitmapOffset);
    result.pixels = pixels;
    result.width = header->width;
    result.height = header->height;

    ASSERT(header->compression == 3);

    u32 redMask = header->redMask;
    u32 greenMask = header->greenMask;
    u32 blueMask = header->blueMask;
    u32 alphaMask = ~(redMask | greenMask | blueMask);

    BitScanResult redShift = findLeastSignificantSetBit(redMask);
    BitScanResult greenShift = findLeastSignificantSetBit(greenMask);
    BitScanResult blueShift = findLeastSignificantSetBit(blueMask);
    BitScanResult alphaShift = findLeastSignificantSetBit(alphaMask);

    ASSERT(redShift.found);
    ASSERT(greenShift.found);
    ASSERT(blueShift.found);
    ASSERT(alphaShift.found);

    u32 *sourceDest = pixels;
    for (s32 y = 0; y < header->height; ++y)
    {
      for (s32 x = 0; x < header->width; ++x)
      {
        u32 c = *sourceDest;
        *sourceDest++ = ((((c >> alphaShift.index) & 0xFF) << 24) |
                       (((c >> redShift.index) & 0xFF) << 16) |
                       (((c >> greenShift.index) & 0xFF) << 8) |
                       (((c >> blueShift.index) & 0xFF) << 0));
      }
    }
  }

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
    gameState->backdrop = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_background.bmp");

    HeroBitmaps *bitmap = gameState->heroBitmaps;

    bitmap->head = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_right_head.bmp");
    bitmap->cape = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_right_cape.bmp");
    bitmap->torso = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_right_torso.bmp");
    bitmap->alignX = 72;
    bitmap->alignY = 182;
    ++bitmap;

    bitmap->head = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_back_head.bmp");
    bitmap->cape = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_back_cape.bmp");
    bitmap->torso = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_back_torso.bmp");
    bitmap->alignX = 72;
    bitmap->alignY = 182;
    ++bitmap;

    bitmap->head = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_left_head.bmp");
    bitmap->cape = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_left_cape.bmp");
    bitmap->torso = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_left_torso.bmp");
    bitmap->alignX = 72;
    bitmap->alignY = 182;
    ++bitmap;

    bitmap->head = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_front_head.bmp");
    bitmap->cape = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_front_cape.bmp");
    bitmap->torso = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_front_torso.bmp");
    bitmap->alignX = 72;
    bitmap->alignY = 182;
    ++bitmap;

    gameState->cameraP.absTileX = 17 / 2;
    gameState->cameraP.absTileY = 9 / 2;

    // do initialization here as needed
    gameState->playerP.absTileX = 1;
    gameState->playerP.absTileY = 3;
    gameState->playerP.offset.x = 5.0f;
    gameState->playerP.offset.y = 5.0f;
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

    tileMap->tileChunkCountX = 128;
    tileMap->tileChunkCountY = 128;
    tileMap->tileChunkCountZ = 2;

    tileMap->tileChunks = PUSH_ARRAY(&gameState->worldArena, tileMap->tileChunkCountX * tileMap->tileChunkCountY * tileMap->tileChunkCountZ, TileChunk);

    u32 randomNumberIndex = 0;
    u32 tilesPerWidth = 17;
    u32 tilesPerHeight = 9;
    u32 screenX = 0;
    u32 screenY = 0;
    u32 absTileZ = 0;

    b32 doorLeft = 0;
    b32 doorRight = 0;
    b32 doorTop = 0;
    b32 doorBottom = 0;
    b32 doorUp = 0;
    b32 doorDown = 0;
    for (u32 screenIndex = 0; screenIndex < 100; screenIndex++)
    {
      ASSERT(randomNumberIndex < ARRAY_COUNT(randomNumberTable));
      u32 randomChoice;
      if (doorUp || doorDown)
      {
        randomChoice = randomNumberTable[randomNumberIndex++] % 2;
      }
      else
      {
        randomChoice = randomNumberTable[randomNumberIndex++] % 3;
      }

      b32 createdZDoor = false;
      if (randomChoice == 2)
      {
        createdZDoor = true;
        if (absTileZ == 0)
        {
          doorUp = 1;
        }
        else
        {
          doorDown = 1;
        }
      }
      else if (randomChoice == 1)
      {
        doorRight = 1;
      }
      else
      {
        doorTop = 1;
      }

      for (u32 tileY = 0; tileY < tilesPerHeight; tileY++)
      {
        for (u32 tileX = 0; tileX < tilesPerWidth; tileX++)
        {
          u32 absTileX = screenX * tilesPerWidth + tileX;
          u32 absTileY = screenY * tilesPerHeight + tileY;

          u32 tileValue = 1;
          if (tileX == 0 && (!doorLeft || tileY != (tilesPerHeight / 2)))
          {
            tileValue = 2;
          }

          if (tileX == tilesPerWidth - 1 && (!doorRight || tileY != (tilesPerHeight / 2)))
          {
              tileValue = 2;
          }

          if (tileY == 0 && (!doorBottom || tileX != (tilesPerWidth / 2)))
          {
              tileValue = 2;
          }

          if (tileY == tilesPerHeight - 1 && (!doorTop || tileX != (tilesPerWidth / 2)))
          {
            tileValue = 2;
          }

          if (tileX == 10 && tileY == 6)
          {
            if (doorUp)
            {
              tileValue = 3;
            }

            if (doorDown)
            {
              tileValue = 4;
            }
          }

          setTileValue(&gameState->worldArena, world->tileMap, absTileX, absTileY, absTileZ, tileValue);
        }
      }

      doorLeft = doorRight;
      doorBottom = doorTop;

      if (createdZDoor)
      {
        doorDown = !doorDown;
        doorUp = !doorUp;
      }
      else
      {
        doorDown = 0;
        doorUp = 0;
      }

      doorRight = 0;
      doorTop = 0;

      if (randomChoice == 2)
      {
        if (absTileZ == 0)
        {
          absTileZ = 1;
        }
        else
        {
          absTileZ = 0;
        }
      }
      else if (randomChoice == 1)
      {
        screenX += 1;
      }
      else
      {
        screenY += 1;
      }
    }

    memory->isInitialized = 1;
  }

  World *world = gameState->world;
  TileMap *tileMap = world->tileMap;

  s32 tileSideInPixels = 60;
  r32 metersToPixels = (r32) tileSideInPixels / tileMap->tileSideInMeters;

  r32 lowerLeftX = -tileSideInPixels / 2;
  r32 lowerLeftY = (r32) buff->height;

  if (gcInput->isAnalog)
  {
  }
  else
  {
    V2 dPlayer = {0};

    if (gcInput->up.endedDown)
    {
      gameState->heroFacingDirection = 1;
      dPlayer.y = 1.f;
    }
    if (gcInput->down.endedDown)
    {
      gameState->heroFacingDirection = 3;
      dPlayer.y = -1.f;
    }
    if (gcInput->left.endedDown)
    {
      gameState->heroFacingDirection = 2;
      dPlayer.x = -1.f;
    }
    if (gcInput->right.endedDown)
    {
      gameState->heroFacingDirection = 0;
      dPlayer.x = 1.f;
    }

    r32 playerSpeed = 2.f;

    if (gcInput->actionUp.endedDown)
    {
      playerSpeed = 10.f;
    }

    dPlayer *= playerSpeed;

    if (dPlayer.x != 0.f && dPlayer.y != 0.f)
    {
      dPlayer *= 0.707106781186548;
    }

    TileMapPosition newPlayerP = gameState->playerP;
    newPlayerP.offset += deltaTimeSec * dPlayer;
    newPlayerP = recanonicalizePosition(tileMap, newPlayerP);

    TileMapPosition playerLeft = newPlayerP;
    playerLeft.offset.x -= 0.5f * playerWidth;
    playerLeft = recanonicalizePosition(tileMap, playerLeft);

    TileMapPosition playerRight = newPlayerP;
    playerRight.offset.x += 0.5f * playerWidth;
    playerRight = recanonicalizePosition(tileMap, playerRight);

    if (isTileMapPointEmpty(tileMap, newPlayerP)
      && isTileMapPointEmpty(tileMap, playerLeft)
      && isTileMapPointEmpty(tileMap, playerRight))
    {
      if (!areOnTheSameTile(&gameState->playerP, &newPlayerP))
      {
        u32 newTileValue = getTileValue(tileMap, newPlayerP);

        if (newTileValue == 3)
        {
          ++newPlayerP.absTileZ;
        }
        else if (newTileValue == 4)
        {
          --newPlayerP.absTileZ;
        }
      }

      gameState->playerP = newPlayerP;
    }

    gameState->cameraP.absTileZ = gameState->playerP.absTileZ;

    TileMapDifference diff = subtract(tileMap, &gameState->playerP, &gameState->cameraP);
    if (diff.dXY.x > 9.f * tileMap->tileSideInMeters)
    {
      gameState->cameraP.absTileX += 17;
    }
    if (diff.dXY.x < -9.f * tileMap->tileSideInMeters)
    {
      gameState->cameraP.absTileX -= 17;
    }
    if (diff.dXY.y > 5.f * tileMap->tileSideInMeters)
    {
      gameState->cameraP.absTileY += 9;
    }
    if (diff.dXY.y < -5.f * tileMap->tileSideInMeters)
    {
      gameState->cameraP.absTileY -= 9;
    }
  }

  drawBitmap(buff, &gameState->backdrop, 0, 0);

  r32 screenCenterX = 0.5f * (r32) buff->width;
  r32 screenCenterY = 0.5f * (r32) buff->height;

  for (s32 relRow = -10; relRow < 10; ++relRow)
  {
    for (s32 relColumn = -20; relColumn < 20; relColumn++)
    {
      u32 column = gameState->cameraP.absTileX + relColumn;
      u32 row = gameState->cameraP.absTileY + relRow;
      u32 tileID = getTileValue(tileMap, column, row, gameState->cameraP.absTileZ);

      if (tileID > 1)
      {
        r32 gray = 0.5f;
        if (tileID == 2)
        {
          gray = 1.f;
        }

        if (tileID > 2)
        {
          gray = 0.25f;
        }

        if (column == gameState->cameraP.absTileX && row == gameState->cameraP.absTileY)
        {
          gray = 0.0f;
        }

        V2 tileSide = {0.5f * tileSideInPixels, 0.5f * tileSideInPixels};
        V2 cen = {screenCenterX - metersToPixels * gameState->cameraP.offset.x + ((r32) relColumn) * tileSideInPixels,
                  screenCenterY + metersToPixels * gameState->cameraP.offset.y - ((r32) relRow) * tileSideInPixels};
        V2 min = cen - tileSide;
        V2 max = cen + tileSide;
        drawRectangle(buff, min, max, gray, gray, gray);
      }
    }
  }

  TileMapDifference diff = subtract(tileMap, &gameState->playerP, &gameState->cameraP);

  r32 playerR = 1.f;
  r32 playerG = 1.f;
  r32 playerB = 0.f;
  r32 playerGroundPointX = screenCenterX + metersToPixels * diff.dXY.x;
  r32 playerGroundPointY = screenCenterY - metersToPixels * diff.dXY.y;
  V2 playerLeftTop = {playerGroundPointX - 0.5f * metersToPixels * playerWidth, playerGroundPointY - metersToPixels * playerHeight};
  V2 playerWidthHeight = {playerWidth, playerHeight};
  drawRectangle(buff, playerLeftTop, playerLeftTop + metersToPixels * playerWidthHeight, playerR, playerG, playerB);

  HeroBitmaps *heroBitmaps = &gameState->heroBitmaps[gameState->heroFacingDirection];
  drawBitmap(buff, &heroBitmaps->torso, playerGroundPointX, playerGroundPointY, heroBitmaps->alignX, heroBitmaps->alignY);
  drawBitmap(buff, &heroBitmaps->cape, playerGroundPointX, playerGroundPointY, heroBitmaps->alignX, heroBitmaps->alignY);
  drawBitmap(buff, &heroBitmaps->head, playerGroundPointX, playerGroundPointY, heroBitmaps->alignX, heroBitmaps->alignY);

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


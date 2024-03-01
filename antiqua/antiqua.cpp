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

static void drawBitmap(GameOffscreenBuffer *buf, LoadedBitmap *bitmap, r32 realX, r32 realY, s32 alignX = 0, s32 alignY = 0, r32 cAlpha = 1.0f)
{
  realX -= (r32) alignX;
  realY -= (r32) alignY;

  s32 minX = roundReal32ToInt32(realX);
  s32 minY = roundReal32ToInt32(realY);
  s32 maxX = minX + bitmap->width;
  s32 maxY = minY + bitmap->height;

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
      a *= cAlpha;
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

inline LowEntity *getLowEntity(GameState *gameState, u32 index)
{
  LowEntity *result = 0;

  if (index > 0 && index < gameState->lowEntityCount)
  {
      result = gameState->lowEntities + index;
  }

  return result;
}

inline HighEntity *makeEntityHighFrequency(GameState *gameState, u32 lowIndex)
{
    HighEntity *entityHigh = 0;

    LowEntity *entityLow = &gameState->lowEntities[lowIndex];
    if (entityLow->highEntityIndex)
    {
        entityHigh = gameState->highEntities_ + entityLow->highEntityIndex;
    }
    else
    {
        if (gameState->highEntityCount < ARRAY_COUNT(gameState->highEntities_))
        {
            u32 highIndex = gameState->highEntityCount++;
            entityHigh = gameState->highEntities_ + highIndex;

            // NOTE(dima): map the entity into camera space
            TileMapDifference diff = subtract(gameState->world->tileMap, &entityLow->p, &gameState->cameraP);
            entityHigh->p = diff.dXY;
            entityHigh->dP = v2(0, 0);
            entityHigh->absTileZ = entityLow->p.absTileZ;
            entityHigh->facingDirection = 0;
            entityHigh->lowEntityIndex = lowIndex;

            entityLow->highEntityIndex = highIndex;
        }
        else
        {
            INVALID_CODE_PATH;
        }
    }

    return entityHigh;
}

inline Entity getHighEntity(GameState *gameState, u32 lowIndex)
{
  Entity result = {};

  makeEntityHighFrequency(gameState, lowIndex);

  if(lowIndex > 0 && lowIndex < gameState->lowEntityCount)
  {
      result.lowIndex = lowIndex;
      result.low = gameState->lowEntities + lowIndex;
      result.high = makeEntityHighFrequency(gameState, lowIndex);
  }

  return result;
}

inline void makeEntityLowFrequency(GameState *gameState, u32 lowIndex)
{
    LowEntity *entityLow = &gameState->lowEntities[lowIndex];
    u32 highIndex = entityLow->highEntityIndex;
    if (highIndex)
    {
        u32 lastHighIndex = gameState->highEntityCount - 1;
        if (highIndex != lastHighIndex)
        {
            HighEntity *lastEntity = gameState->highEntities_ + lastHighIndex;
            HighEntity *delEntity = gameState->highEntities_ + highIndex;

            *delEntity = *lastEntity;
            gameState->lowEntities[lastEntity->lowEntityIndex].highEntityIndex = highIndex;
        }
        --gameState->highEntityCount;
        entityLow->highEntityIndex = 0;
    }
}

inline void offsetAndCheckFrequencyByArea(GameState *gameState, V2 offset, Rectangle2 cameraBounds)
{
    for (u32 entityIndex = 1;
        entityIndex < gameState->highEntityCount;
        )
    {
        HighEntity *high = gameState->highEntities_ + entityIndex;

        high->p += offset;
        if (isInRectangle(cameraBounds, high->p))
        {
            ++entityIndex;
        }
        else
        {
            makeEntityLowFrequency(gameState, high->lowEntityIndex);
        }
    }

}

static u32 addLowEntity(GameState *gameState, EntityType type)
{
    ASSERT(gameState->lowEntityCount < ARRAY_COUNT(gameState->lowEntities));
    u32 entityIndex = gameState->lowEntityCount++;

    gameState->lowEntities[entityIndex] = {};
    gameState->lowEntities[entityIndex].type = type;

    return entityIndex;
}

static u32 addWall(GameState *gameState, u32 absTileX, u32 absTileY, u32 absTileZ)
{
    u32 entityIndex = addLowEntity(gameState, EntityType_Wall);
    LowEntity *entityLow = getLowEntity(gameState, entityIndex);

    entityLow->p.absTileX = absTileX;
    entityLow->p.absTileY = absTileY;
    entityLow->p.absTileZ = absTileZ;
    entityLow->height = gameState->world->tileMap->tileSideInMeters;//1.4f;
    entityLow->width = entityLow->height;
    entityLow->collides = true;

    return entityIndex;
}

static u32 addPlayer(GameState *gameState)
{
    u32 entityIndex = addLowEntity(gameState, EntityType_Hero);
    LowEntity *entityLow = getLowEntity(gameState, entityIndex);

    entityLow->p.absTileX = 1;
    entityLow->p.absTileY = 3;
    entityLow->p.offset_.x = 0;
    entityLow->p.offset_.y = 0;
    entityLow->height = 0.5f;//1.4f;
    entityLow->width = 1.0f;
    entityLow->collides = true;

    if (gameState->cameraFollowingEntityIndex == 0)
    {
        gameState->cameraFollowingEntityIndex = entityIndex;
    }

    return entityIndex;
}

static b32 testWall(r32 wallX, r32 relX, r32 relY, r32 playerDeltaX,
r32 playerDeltaY, r32 *tMin, r32 minY, r32 maxY)
{
  b32 hit = false;

  r32 tEpsilon = 0.001f;
  if (playerDeltaX != 0.0f)
  {
    r32 tResult = (wallX - relX) / playerDeltaX;
    r32 y = relY + tResult * playerDeltaY;
    if (tResult >= 0.0f && *tMin > tResult)
    {
      if (y >= minY && y <= maxY)
      {
        *tMin = MAXIMUM(0.0f, tResult - tEpsilon);
        hit = true;
      }
    }
  }

  return hit;
}

static void movePlayer(GameState *gameState, Entity entity, r32 dt, V2 ddP)
{
  TileMap *tileMap = gameState->world->tileMap;

  r32 ddPLength = lengthSq(ddP);
  if (ddPLength > 1.0f)
  {
    ddP *= (1.0f / squareRoot(ddPLength));
  }

  r32 playerSpeed = 50.f;
  ddP *= playerSpeed;

  ddP += -8.0f * entity.high->dP;

  V2 oldPlayerP = entity.high->p;
  V2 playerDelta = 0.5f * ddP * square(dt) + entity.high->dP * dt;
  entity.high->dP = ddP * dt + entity.high->dP;
  V2 newPlayerP = oldPlayerP + playerDelta;

  /*
  u32 minTileX = MINIMUM(oldPlayerP.absTileX, newPlayerP.absTileX);
  u32 minTileY = MINIMUM(oldPlayerP.absTileY, newPlayerP.absTileY);
  u32 maxTileX = MAXIMUM(oldPlayerP.absTileX, newPlayerP.absTileX);
  u32 maxTileY = MAXIMUM(oldPlayerP.absTileY, newPlayerP.absTileY);

  u32 entityTileWidth = ceilReal32ToInt32(entity.high->width / tileMap->tileSideInMeters);
  u32 entityTileHeight = ceilReal32ToInt32(entity.high->height / tileMap->tileSideInMeters);

  minTileX -= entityTileWidth;
  minTileY -= entityTileHeight;
  maxTileX += entityTileWidth;
  maxTileY += entityTileHeight;

  u32 absTileZ = entity.high->p.absTileZ;
  */

  for (u32 iteration = 0;
       iteration < 4;
       ++iteration)
  {
      r32 tMin = 1.0f;
      V2 wallNormal = {0};
      u32 hitHighEntityIndex = 0;

      V2 desiredPosition = entity.high->p + playerDelta;

      for (u32 testHighEntityIndex = 1; testHighEntityIndex < gameState->highEntityCount; ++testHighEntityIndex)
      {
          if (testHighEntityIndex != entity.low->highEntityIndex)
          {
              Entity testEntity;
              testEntity.high = gameState->highEntities_ + testHighEntityIndex; 
              testEntity.lowIndex = testEntity.high->lowEntityIndex;
              testEntity.low = gameState->lowEntities + testEntity.lowIndex;
              if (testEntity.low->collides)
              {
                  r32 diameterW = testEntity.low->width + entity.low->width;
                  r32 diameterH = testEntity.low->height + entity.low->height;
                  V2 minCorner = -0.5f * V2 { diameterW, diameterH };
                  V2 maxCorner = 0.5f * V2 { diameterW, diameterH };

                  V2 rel = entity.high->p - testEntity.high->p;

                  if (testWall(minCorner.x, rel.x, rel.y, playerDelta.x, playerDelta.y, &tMin, minCorner.y, maxCorner.y))
                  {
                      wallNormal = v2(-1, 0);
                      hitHighEntityIndex = testHighEntityIndex;
                  }
                  if (testWall(maxCorner.x, rel.x, rel.y, playerDelta.x, playerDelta.y, &tMin, minCorner.y, maxCorner.y))
                  {
                      wallNormal = v2(1, 0);
                      hitHighEntityIndex = testHighEntityIndex;
                  }
                  if (testWall(minCorner.y, rel.y, rel.x, playerDelta.y, playerDelta.x, &tMin, minCorner.x, maxCorner.x))
                  {
                      wallNormal = v2(0, -1);
                      hitHighEntityIndex = testHighEntityIndex;
                  }
                  if (testWall(maxCorner.y, rel.y, rel.x, playerDelta.y, playerDelta.x, &tMin, minCorner.x, maxCorner.x))
                  {
                      wallNormal = v2(0, 1);
                      hitHighEntityIndex = testHighEntityIndex;
                  }
              }
          }
      }

      entity.high->p += tMin * playerDelta;
      if (hitHighEntityIndex)
      {
          entity.high->dP = entity.high->dP - 1 * inner(entity.high->dP, wallNormal) * wallNormal;
          playerDelta = desiredPosition - entity.high->p;
          playerDelta = playerDelta - 1 * inner(playerDelta, wallNormal) * wallNormal; 

          HighEntity *hitHigh = gameState->highEntities_ + hitHighEntityIndex;
          LowEntity *hitLow = gameState->lowEntities + hitHigh->lowEntityIndex;
          entity.high->absTileZ += hitLow->dAbsTileZ;
      }
      else
      {
          break;
      }
  }

  if (entity.high->dP.x == 0.0f && entity.high->dP.y == 0.0f)
  {
    // NOTE(dima): leave facingDirection whatever it was
  }
  else
  {
    if (absoluteValue(entity.high->dP.x) > absoluteValue(entity.high->dP.y))
    {
      if (entity.high->dP.x > 0)
      {
        entity.high->facingDirection = 0;
      }
      else
      {
        entity.high->facingDirection = 2;
      }
    }
    else if (absoluteValue(entity.high->dP.x) < absoluteValue(entity.high->dP.y))
    {
      if (entity.high->dP.y > 0)
      {
        entity.high->facingDirection = 1;
      }
      else
      {
        entity.high->facingDirection = 3;
      }
    }
  }

  entity.low->p = mapIntoTileSpace(gameState->world->tileMap, gameState->cameraP, entity.high->p);
}

static void
setCamera(GameState *gameState, TileMapPosition newCameraP)
{
    TileMap *tileMap = gameState->world->tileMap;

    TileMapDifference dCameraP = subtract(tileMap, &newCameraP, &gameState->cameraP);
    gameState->cameraP = newCameraP;

    u32 tileSpanX = 17 * 3;
    u32 tileSpanY = 9 * 3;
    Rectangle2 cameraBounds = rectCenterHalfDim(v2(0, 0), tileMap->tileSideInMeters * v2((r32) tileSpanX, (r32) tileSpanY));

    V2 entityOffsetForFrame = -dCameraP.dXY;
    offsetAndCheckFrequencyByArea(gameState, entityOffsetForFrame, cameraBounds);

    u32 minTileX = newCameraP.absTileX - tileSpanX / 2;
    u32 maxTileX = newCameraP.absTileX + tileSpanX / 2;
    u32 minTileY = newCameraP.absTileY - tileSpanY / 2;
    u32 maxTileY = newCameraP.absTileY + tileSpanY / 2;
    for (u32 entityIndex = 1;
        entityIndex < gameState->lowEntityCount;
        ++entityIndex)
    {
        LowEntity *low = gameState->lowEntities + entityIndex;
        if (low->highEntityIndex == 0)
        {
            if (low->p.absTileZ == newCameraP.absTileZ &&
                low->p.absTileX >= minTileX &&
                low->p.absTileX <= maxTileX &&
                low->p.absTileY <= minTileY &&
                low->p.absTileY >= maxTileY)
            {
                makeEntityHighFrequency(gameState, entityIndex);
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
        // NOTE(dima): reserve entity slot 0 for the null entity
        addLowEntity(gameState, EntityType_Null);
        gameState->highEntityCount = 1;

        gameState->backdrop = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_background.bmp");
        gameState->shadow = Debug_LoadBMP(thread, memory->debug_platformReadEntireFile, "data/test/test_hero_shadow.bmp");

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

        // do initialization here as needed

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
#if 0
        // TODO(dima): waiting for full sparseness
        u32 screenX = INT32_MAX / 2;
        u32 screenY = INT32_MAX / 2;
#else
        u32 screenX = 0;
        u32 screenY = 0;
#endif
        u32 absTileZ = 0;

        b32 doorLeft = 0;
        b32 doorRight = 0;
        b32 doorTop = 0;
        b32 doorBottom = 0;
        b32 doorUp = 0;
        b32 doorDown = 0;
        for (u32 screenIndex = 0; screenIndex < 2; screenIndex++)
        {
            ASSERT(randomNumberIndex < ARRAY_COUNT(randomNumberTable));
            u32 randomChoice;
//            if (doorUp || doorDown)
            {
                randomChoice = randomNumberTable[randomNumberIndex++] % 2;
            }
#if 0
            else
            {
                randomChoice = randomNumberTable[randomNumberIndex++] % 3;
            }
#endif

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

                    if (tileValue == 2)
                    {
                        addWall(gameState, absTileX, absTileY, absTileZ);
                    }
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

        TileMapPosition newCameraP = {};
        newCameraP.absTileX = 17 / 2;
        newCameraP.absTileY = 9 / 2;
        setCamera(gameState, newCameraP);

        memory->isInitialized = 1;
    }

    World *world = gameState->world;
    TileMap *tileMap = world->tileMap;

    s32 tileSideInPixels = 60;
    r32 metersToPixels = (r32) tileSideInPixels / tileMap->tileSideInMeters;

    r32 lowerLeftX = -tileSideInPixels / 2;
    r32 lowerLeftY = (r32) buff->height;

    u32 lowIndex = gameState->playerIndexForController[0];
    if (lowIndex == 0)
    {
        u32 entityIndex = addPlayer(gameState);
        gameState->playerIndexForController[0] = entityIndex;
    }
    else
    {
        Entity controllingEntity = getHighEntity(gameState, lowIndex);
        V2 ddP = {0};

        if (gcInput->isAnalog)
        {
            // TODO(dima): need to normalize stickAverage values for it to work
            ddP = V2 { gcInput->stickAverageX, gcInput->stickAverageY };
        }
        else
        {
            if (gcInput->up.endedDown)
            {
                ddP.y = 1.f;
            }
            if (gcInput->down.endedDown)
            {
                ddP.y = -1.f;
            }
            if (gcInput->left.endedDown)
            {
                ddP.x = -1.f;
            }
            if (gcInput->right.endedDown)
            {
                ddP.x = 1.f;
            }
            if (gcInput->actionUp.endedDown)
            {
                controllingEntity.high->dZ = 3.0f;
            }
        }

        movePlayer(gameState, controllingEntity, deltaTimeSec, ddP);
    }

    V2 entityOffsetForFrame = {};
    Entity cameraFollowingEntity = getHighEntity(gameState, gameState->cameraFollowingEntityIndex);
    if (cameraFollowingEntity.high)
    {
        TileMapPosition newCameraP = gameState->cameraP;

        newCameraP.absTileZ = cameraFollowingEntity.low->p.absTileZ;

#if 1
        if (cameraFollowingEntity.high->p.x > 9.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileX += 17;
        }
        if (cameraFollowingEntity.high->p.x < -9.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileX -= 17;
        }
        if (cameraFollowingEntity.high->p.y > 5.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileY += 9;
        }
        if (cameraFollowingEntity.high->p.y < -5.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileY -= 9;
        }
#else
        if (cameraFollowingEntity.high->p.x > 1.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileX += 1;
        }
        if (cameraFollowingEntity.high->p.x < -1.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileX -= 1;
        }
        if (cameraFollowingEntity.high->p.y > 1.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileY += 1;
        }
        if (cameraFollowingEntity.high->p.y < -1.f * tileMap->tileSideInMeters)
        {
            newCameraP.absTileY -= 1;
        }
#endif

        setCamera(gameState, newCameraP);
    }

    // NOTE(dima): render
    drawBitmap(buff, &gameState->backdrop, 0, 0);

    r32 screenCenterX = 0.5f * (r32) buff->width;
    r32 screenCenterY = 0.5f * (r32) buff->height;

#if 0
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
                V2 cen = {screenCenterX - metersToPixels * gameState->cameraP.offset_.x + ((r32) relColumn) * tileSideInPixels,
                          screenCenterY + metersToPixels * gameState->cameraP.offset_.y - ((r32) relRow) * tileSideInPixels};
                V2 min = cen - 0.9f * tileSide;
                V2 max = cen + 0.9f * tileSide;
                drawRectangle(buff, min, max, gray, gray, gray);
            }
        }
    }
#endif

    for (u32 highEntityIndex = 1; 
    highEntityIndex < gameState->highEntityCount;
    ++highEntityIndex)
    {
        HighEntity *highEntity = gameState->highEntities_ + highEntityIndex;
        LowEntity *lowEntity = gameState->lowEntities + highEntity->lowEntityIndex;

        highEntity->p += entityOffsetForFrame;

        r32 ddZ = -9.8f;
        highEntity->z = 0.5f * ddZ * square(deltaTimeSec) + highEntity->dZ * deltaTimeSec + highEntity->z;
        highEntity->dZ = ddZ * deltaTimeSec + highEntity->dZ;
        if (highEntity->z < 0)
        {
            highEntity->z = 0;
        }
        r32 cAlpha = 1.0f - 0.5f * highEntity->z;
        if (cAlpha < 0)
        {
            cAlpha = 0.0f;
        }

        r32 playerR = 1.f;
        r32 playerG = 1.f;
        r32 playerB = 0.f;
        r32 playerGroundPointX = screenCenterX + metersToPixels * highEntity->p.x;
        r32 playerGroundPointY = screenCenterY - metersToPixels * highEntity->p.y;
        r32 z = -metersToPixels * highEntity->z;
        V2 playerLeftTop = {playerGroundPointX - 0.5f * metersToPixels * lowEntity->width, playerGroundPointY - 0.5f * metersToPixels * lowEntity->height};
        V2 entityWidthHeight = {lowEntity->width, lowEntity->height};

        if (lowEntity->type == EntityType_Hero)
        {
            HeroBitmaps *heroBitmaps = &gameState->heroBitmaps[highEntity->facingDirection];
            drawBitmap(buff, &gameState->shadow, playerGroundPointX, playerGroundPointY, heroBitmaps->alignX, heroBitmaps->alignY, cAlpha);
            drawBitmap(buff, &heroBitmaps->torso, playerGroundPointX, playerGroundPointY + z, heroBitmaps->alignX, heroBitmaps->alignY);
            drawBitmap(buff, &heroBitmaps->cape, playerGroundPointX, playerGroundPointY + z, heroBitmaps->alignX, heroBitmaps->alignY);
            drawBitmap(buff, &heroBitmaps->head, playerGroundPointX, playerGroundPointY + z, heroBitmaps->alignX, heroBitmaps->alignY);
        }
        else
        {
            drawRectangle(buff, 
                          playerLeftTop,
                          playerLeftTop + metersToPixels * entityWidthHeight,
                          playerR, playerG, playerB);
        }
    }

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


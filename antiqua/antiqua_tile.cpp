#include "antiqua_tile.h"

inline TileChunk * getTileChunk(TileMap *tileMap, u32 tileChunkX, u32 tileChunkY, u32 tileChunkZ)
{
  TileChunk *tileChunk = 0;

  if (tileChunkX >= 0 && tileChunkX < tileMap->tileChunkCountX &&
    tileChunkY >= 0 && tileChunkY < tileMap->tileChunkCountY &&
    tileChunkZ >= 0 && tileChunkZ < tileMap->tileChunkCountZ)
  {
    tileChunk = &tileMap->tileChunks[
      tileChunkZ * tileMap->tileChunkCountY * tileMap->tileChunkCountX +
      tileChunkY * tileMap->tileChunkCountX + 
      tileChunkX];
  }

  return tileChunk;
}

inline u32 getTileValueUnchecked(TileMap *tileMap, TileChunk *tileChunk, u32 tileX, u32 tileY)
{
  ASSERT(tileChunk);
  ASSERT(tileX < tileMap->chunkDim);
  ASSERT(tileY < tileMap->chunkDim);

  u32 tileChunkValue = tileChunk->tiles[tileY * tileMap->chunkDim + tileX];
  return tileChunkValue;
}

inline void setTileValueUnchecked(TileMap *tileMap, TileChunk *tileChunk, u32 tileX, u32 tileY, u32 tileValue)
{
  ASSERT(tileChunk);
  ASSERT(tileX < tileMap->chunkDim);
  ASSERT(tileY < tileMap->chunkDim);

  tileChunk->tiles[tileY * tileMap->chunkDim + tileX] = tileValue;
}

inline void setTileValue(TileMap *tileMap, TileChunk *tileChunk, u32 testTileX, u32 testTileY, u32 tileValue)
{
  if (tileChunk && tileChunk->tiles)
  {
    setTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY, tileValue);
  }
}

inline TileChunkPosition getChunkPositionFor(TileMap *tileMap, u32 absTileX, u32 absTileY, u32 absTileZ)
{
  TileChunkPosition result;

  result.tileChunkX = absTileX >> tileMap->chunkShift;
  result.tileChunkY = absTileY >> tileMap->chunkShift;
  result.tileChunkZ = absTileZ;
  result.relTileX = absTileX & tileMap->chunkMask;
  result.relTileY = absTileY & tileMap->chunkMask;

  return result;
}

inline u32 getTileValue(TileMap *tileMap, TileChunk *tileChunk, u32 testTileX, u32 testTileY)
{
  u32 tileChunkValue = 0;

  if (tileChunk && tileChunk->tiles)
  {
    tileChunkValue = getTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY);
  }

  return tileChunkValue;
}

static inline u32 getTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY, u32 absTileZ)
{
  TileChunkPosition chunkPos = getChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
  TileChunk *tileChunk = getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY, chunkPos.tileChunkZ);
  u32 tileChunkValue = getTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY);

  return tileChunkValue;
}

static inline u32 getTileValue(TileMap *tileMap, TileMapPosition pos)
{
  u32 tileChunkValue = getTileValue(tileMap, pos.absTileX, pos.absTileY, pos.absTileZ);
  
  return tileChunkValue;
}

static b32 isTileValueEmpty(u32 tileValue)
{
  b32 empty = (tileValue == 1) || (tileValue == 3) || (tileValue == 4);

  return empty;
}

static b32 isTileMapPointEmpty(TileMap *tileMap, TileMapPosition pos)
{
  u32 tileChunkValue = getTileValue(tileMap, pos);
  b32 empty = isTileValueEmpty(tileChunkValue);

  return empty;
}

static void setTileValue(MemoryArena *arena, TileMap *tileMap, u32 absTileX, u32 absTileY, u32 absTileZ, u32 tileValue)
{
  TileChunkPosition chunkPos = getChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
  TileChunk *tileChunk = getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY, chunkPos.tileChunkZ);

  ASSERT(tileChunk);

  if (!tileChunk->tiles)
  {
      u32 tileCount = tileMap->chunkDim * tileMap->chunkDim;
      tileChunk->tiles = PUSH_ARRAY(arena, tileCount, u32);
      for (u32 tileIndex = 0; tileIndex < tileCount; tileIndex++)
      {
        tileChunk->tiles[tileIndex] = 1;
      }
  }

  setTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY, tileValue);
}

//
//
//

inline void recanonicalizeCoord(TileMap *tileMap, u32 *tile, r32 *tileRel)
{
  s32 offset = roundReal32ToInt32(*tileRel / tileMap->tileSideInMeters);
  *tile += offset;
  *tileRel -= offset * tileMap->tileSideInMeters;

  ASSERT(*tileRel > -0.5f * tileMap->tileSideInMeters);
  ASSERT(*tileRel < 0.5f * tileMap->tileSideInMeters);
}

inline TileMapPosition mapIntoTileSpace(TileMap* tileMap, TileMapPosition basePos, V2 offset)
{
  TileMapPosition result = basePos;

  result.offset_ += offset;
  recanonicalizeCoord(tileMap, &result.absTileX, &result.offset_.x);
  recanonicalizeCoord(tileMap, &result.absTileY, &result.offset_.y);

  return result;
}

inline b32 areOnTheSameTile(TileMapPosition *a, TileMapPosition *b)
{
  b32 result = a->absTileX == b->absTileX && a->absTileY == b->absTileY && a->absTileZ == b->absTileZ;

  return result;
}

inline TileMapDifference subtract(TileMap *tileMap, TileMapPosition *a, TileMapPosition *b)
{
  TileMapDifference result;

  V2 dTileXY = {(r32) a->absTileX - (r32) b->absTileX,
                (r32) a->absTileY - (r32) b->absTileY};
  r32 dTileZ = (r32) a->absTileZ - (r32) b->absTileZ;

  result.dXY = tileMap->tileSideInMeters * dTileXY + (a->offset_- b->offset_);
  result.dz = tileMap->tileSideInMeters * dTileZ;

  return result;
}

inline TileMapPosition centeredTilePoint(u32 absTileX, u32 absTileY, u32 absTileZ)
{
  TileMapPosition result = {0};

  result.absTileX = absTileX;
  result.absTileY = absTileY;
  result.absTileZ = absTileZ;

  return result;
}

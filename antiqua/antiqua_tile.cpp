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

inline u32 getTileValue(TileMap *tileMap, TileChunk *tileChunk, u32 testTileX, u32 testTileY)
{
  u32 tileChunkValue = 0;

  if (tileChunk && tileChunk->tiles)
  {
    tileChunkValue = getTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY);
  }

  return tileChunkValue;
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

static u32 getTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY, u32 absTileZ)
{
  TileChunkPosition chunkPos = getChunkPositionFor(tileMap, absTileX, absTileY, absTileZ);
  TileChunk *tileChunk = getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY, chunkPos.tileChunkZ);
  u32 tileChunkValue = getTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY);

  return tileChunkValue;
}

static u32 getTileValue(TileMap *tileMap, TileMapPosition pos)
{
  u32 tileChunkValue = getTileValue(tileMap, pos.absTileX, pos.absTileY, pos.absTileZ);
  
  return tileChunkValue;
}

static b32 isTileMapPointEmpty(TileMap *tileMap, TileMapPosition pos)
{
  u32 tileChunkValue = getTileValue(tileMap, pos);
  b32 empty = (tileChunkValue == 1) || (tileChunkValue == 3) || (tileChunkValue == 4);

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

  ASSERT(*tileRel >= -0.5f * tileMap->tileSideInMeters);
  ASSERT(*tileRel <= 0.5f * tileMap->tileSideInMeters);
}

inline TileMapPosition recanonicalizePosition(TileMap* tileMap, TileMapPosition pos)
{
  TileMapPosition result = pos;

  recanonicalizeCoord(tileMap, &result.absTileX, &result.offsetX);
  recanonicalizeCoord(tileMap, &result.absTileY, &result.offsetY);

  return result;
}

inline b32 areOnTheSameTile(TileMapPosition *a, TileMapPosition *b)
{
  b32 result = a->absTileX == b->absTileX && a->absTileY == b->absTileY && a->absTileZ == b->absTileZ;

  return result;
}

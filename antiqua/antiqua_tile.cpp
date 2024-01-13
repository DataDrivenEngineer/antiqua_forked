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

  recanonicalizeCoord(tileMap, &result.absTileX, &result.tileRelX);
  recanonicalizeCoord(tileMap, &result.absTileY, &result.tileRelY);

  return result;
}

inline TileChunk * getTileChunk(TileMap *tileMap, u32 tileChunkX, u32 tileChunkY)
{
  TileChunk *tileChunk = 0;

  if (tileChunkX >= 0 && tileChunkX < tileMap->tileChunkCountX &&
    tileChunkY >= 0 && tileChunkY < tileMap->tileChunkCountY)
  {
    tileChunk = &tileMap->tileChunks[tileChunkY * tileMap->tileChunkCountX + tileChunkX];
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

  if (tileChunk)
  {
    tileChunkValue = getTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY);
  }

  return tileChunkValue;
}

inline void setTileValue(TileMap *tileMap, TileChunk *tileChunk, u32 testTileX, u32 testTileY, u32 tileValue)
{
  if (tileChunk)
  {
    setTileValueUnchecked(tileMap, tileChunk, testTileX, testTileY, tileValue);
  }
}

inline TileChunkPosition getChunkPositionFor(TileMap *tileMap, u32 absTileX, u32 absTileY)
{
  TileChunkPosition result;

  result.tileChunkX = absTileX >> tileMap->chunkShift;
  result.tileChunkY = absTileY >> tileMap->chunkShift;
  result.relTileX = absTileX & tileMap->chunkMask;
  result.relTileY = absTileY & tileMap->chunkMask;

  return result;
}

u32 getTileValue(TileMap *tileMap, u32 absTileX, u32 absTileY)
{
  TileChunkPosition chunkPos = getChunkPositionFor(tileMap, absTileX, absTileY);
  TileChunk *tileChunk = getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY);
  u32 tileChunkValue = getTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY);

  return tileChunkValue;
}

b32 isTileMapPointEmpty(TileMap *tileMap, TileMapPosition canPos)
{
  u32 tileChunkValue = getTileValue(tileMap, canPos.absTileX, canPos.absTileY);
  b32 empty = (tileChunkValue == 0);

  return empty;
}

void setTileValue(MemoryArena *arena, TileMap *tileMap, u32 absTileX, u32 absTileY, u32 tileValue)
{
  TileChunkPosition chunkPos = getChunkPositionFor(tileMap, absTileX, absTileY);
  TileChunk *tileChunk = getTileChunk(tileMap, chunkPos.tileChunkX, chunkPos.tileChunkY);

  ASSERT(tileChunk);
  setTileValue(tileMap, tileChunk, chunkPos.relTileX, chunkPos.relTileY, tileValue);
}

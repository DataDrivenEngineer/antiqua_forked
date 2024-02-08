#ifndef _ANTIQUA_TILE_H_
#define _ANTIQUA_TILE_H_

#include "antiqua_math.h"

typedef struct
{
  V2 dXY;
  r32 dz;
} TileMapDifference;

typedef struct
{
  u32 absTileX;
  u32 absTileY;
  u32 absTileZ;

  // NOTE(dima): these are offsets from tile center
  V2 offset;
} TileMapPosition;

typedef struct
{
  u32 tileChunkX;
  u32 tileChunkY;
  u32 tileChunkZ;

  u32 relTileX;
  u32 relTileY;
} TileChunkPosition;

typedef struct
{
  u32 *tiles;
} TileChunk;

typedef struct
{
  u32 chunkShift;
  u32 chunkMask;
  u32 chunkDim;

  r32 tileSideInMeters;

  u32 tileChunkCountX;
  u32 tileChunkCountY;
  u32 tileChunkCountZ;

  TileChunk *tileChunks;
} TileMap;

#endif

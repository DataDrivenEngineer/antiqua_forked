#ifndef _ANTIQUA_TILE_H_
#define _ANTIQUA_TILE_H_

typedef struct TileMapPosition
{
  u32 absTileX;
  u32 absTileY;

  // NOTE(dima): this is tile-relative X and Y
  r32 tileRelX;
  r32 tileRelY;
} TileMapPosition;

typedef struct TileChunkPosition
{
  u32 tileChunkX;
  u32 tileChunkY;

  u32 relTileX;
  u32 relTileY;
} TileChunkPosition;

typedef struct TileChunk
{
  u32 *tiles;
} TileChunk;

typedef struct TileMap
{
  u32 chunkShift;
  u32 chunkMask;
  u32 chunkDim;

  r32 tileSideInMeters;
  s32 tileSideInPixels;
  r32 metersToPixels;

  u32 tileChunkCountX;
  u32 tileChunkCountY;

  TileChunk *tileChunks;
} TileMap;

#endif

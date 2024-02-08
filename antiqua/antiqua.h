#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "antiqua_platform.h"
#include "antiqua_tile.h"

void initializeArena(MemoryArena *arena, MemoryIndex size, u8 *base)
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

typedef struct
{
  TileMap *tileMap;
} World;

typedef struct
{
  s32 width;
  s32 height;
  u32 *pixels;
} LoadedBitmap;

typedef struct
{
  s32 alignX;
  s32 alignY;
  LoadedBitmap head;
  LoadedBitmap cape;
  LoadedBitmap torso;
} HeroBitmaps;

typedef struct
{
  MemoryArena worldArena;
  World *world;

  TileMapPosition cameraP;
  TileMapPosition playerP;

  LoadedBitmap backdrop;
  u32 heroFacingDirection;
  HeroBitmaps heroBitmaps[4];
} GameState;

#endif

#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "antiqua_platform.h"
#include "antiqua_math.h"

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
  v2 p; // NOTE(dima): already relative to the camera
  v2 dP;
  u32 facingDirection;
  u32 absTileZ;

  r32 z;
  r32 dZ;

  u32 lowEntityIndex;
} HighEntity;

enum EntityType
{
    EntityType_Null,

    EntityType_Hero,
    EntityType_Wall,
};

typedef struct
{
    EntityType type;

    r32 width;
    r32 height;

    // NOTE(dima): this is for "stairs"
    b32 collides;
    s32 dAbsTileZ;

    u32 highEntityIndex;
} LowEntity;

typedef struct
{
  u32 lowIndex;
  LowEntity *low;
  HighEntity *high;
} Entity;

typedef struct
{
  MemoryArena worldArena;
  World *world;

  u32 cameraFollowingEntityIndex;

  u32 playerIndexForController[1];

  u32 lowEntityCount;
  LowEntity lowEntities[100000];

  u32 highEntityCount;
  HighEntity highEntities_[256];

  LoadedBitmap backdrop;
  LoadedBitmap shadow;
  HeroBitmaps heroBitmaps[4];
} GameState;

#endif

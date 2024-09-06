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
#define PUSH_SIZE(arena, size, Type) (Type *) pushSize_(arena, (size))
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

enum EntityType
{
    EntityType_Null,
    EntityType_Cube,
};

typedef struct
{
    EntityType type;
    V3 positionWorld;
} Entity;

typedef struct
{
    MemoryArena worldArena;
    World *world;

    u32 cameraFollowingEntityIndex;

    u32 entityCount;
    Entity entities[256];

    M44 worldMatrix;
    M44 viewMatrix;
    M44 projectionMatrix;

    r32 near;
    r32 far;
    r32 fov;

    r32 cameraMinDistance;
    r32 cameraMaxDistance;
    r32 cameraCurrDistance;

    r32 cameraIsometricVerticalAxisRotationAngleDegrees;
    r32 cameraIsometricHorizontalAxisRotationAngleDegrees;
    r32 cameraVerticalAngle;
    r32 cameraHorizontalAngle;
    r32 cameraRotationSpeed;
    r32 cameraMovementSpeed;
    V3 cameraPosWorld;
    V3 v;
    V3 n;
    V3 u;
    V3 isometricV;
    V3 isometricN;
    V3 isometricU;
    b32 debugCameraEnabled;

    V3 tilemapOriginPositionWorld;
    u32 tileCountPerSide;
    r32 tileSideLength;

    b32 isClickPositionInsideTilemap;

    s32 mousePos[2];

    V3 cameraDirectonPosVectorStartWorld;
    V3 cameraDirectonPosVectorEndWorld;
    V3 mouseDirectionVectorWorld;
    V3 mouseDirectionVectorHorizontalWorld;
    V3 mouseDirectionVectorVerticalWorld;
} GameState;

#endif

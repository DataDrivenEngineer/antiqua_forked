#ifndef _ANTIQUA_H_

#include "antiqua_platform.h"
#include "antiqua_math.h"

typedef struct Rect
{
    r32 diameterW;
    r32 diameterH;
} Rect;

enum EntityFlags
{
    EntityFlags_Static = 1 << 0,
    EntityFlags_Moving = 1 << 1,

    EntityFlags_PlayerControlled = 1 << 10,
    EntityFlags_Enemy = 1 << 11,
};

typedef struct Entity
{
    EntityFlags flags;

    V3 posWorld;
    V3 dPos;
    V3 scaleFactor;
} Entity;

typedef struct AssetHeader
{
    struct AssetHeader *next;
    u32 width;
    u32 height;
    u32 pixelSizeBytes;
} AssetHeader;

typedef struct GlyphMetadata
{
    u32 atlasRowOffset;
    u32 atlasColumnOffset;
    u32 glyphWidth;
    u32 glyphHeight;
} GlyphMetadata;

typedef struct Font
{
    GlyphMetadata *glyphMetadata;
    AssetHeader *atlasHeader;
    u8 firstGlyphCode;
    u8 lastGlyphCode;
    u32 glyphCount;
    b32 needsGpuReupload;
} Font;

typedef struct GameState
{
    MemoryArena worldArena;

    Font font;

    u32 cameraFollowingEntityIndex;

    u32 entityCount;
    Entity entities[256];

    M44 worldMatrix;
    M44 projectionMatrix;

    r32 nearPlane;
    r32 farPlane;
    r32 fov;

    r32 cameraMinDistance;
    r32 cameraMaxDistance;
    r32 cameraCurrDistance;

    r32 playerSpeed;

    r32 cameraIsometricVerticalAxisRotationAngleDegrees;
    r32 cameraIsometricHorizontalAxisRotationAngleDegrees;
    r32 cameraVerticalAngle;
    r32 cameraHorizontalAngle;
    r32 cameraRotationSpeed;
    r32 cameraMovementSpeed;
    V3 cameraPosWorld;
    V3 cameraDPos;
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

    r32 mousePos[2];

    V3 cameraDirectonPosVectorStartWorld;
    V3 cameraDirectonPosVectorEndWorld;
    V3 mouseDirectionVectorWorld;
    V3 mouseDirectionVectorHorizontalWorld;
    V3 mouseDirectionVectorVerticalWorld;

    // NOTE(dima): collision detection temporary variable
    r32 prevTimeOfCollision;

#if ANTIQUA_INTERNAL
    V3 boundingBoxPoints[4];
    V3 collisionPointDebug;
    V3 lineNormalVector;
#endif

    M44 viewMatrix;
} GameState;

#define _ANTIQUA_H_
#endif

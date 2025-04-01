#ifndef _ANTIQUA_RENDER_GROUP_H_
#define _ANTIQUA_RENDER_GROUP_H_

#include "types.h"
#include "antiqua_math.h"

typedef enum
{
    RenderGroupEntryType_RenderEntryClear,
    RenderGroupEntryType_RenderEntryPoint,
    RenderGroupEntryType_RenderEntryLine,
    RenderGroupEntryType_RenderEntryMesh,
    RenderGroupEntryType_RenderEntryTile
} RenderGroupEntryType;

typedef struct RenderGroupEntryHeader
{
    RenderGroupEntryType type;
    RenderGroupEntryHeader *next;
} RenderGroupEntryHeader;

typedef struct
{
    V4 color;
} RenderEntryClear;

typedef struct
{
    V3 position;
    V3 color;
} RenderEntryPoint;

typedef struct
{
    /* NOTE(dima): index 0 - start, index 1 - end */
    V3 color;
    V3 start;
    V3 end;
} RenderEntryLine;

typedef struct RenderEntryMesh
{
    M44 modelMatrix;

    u32 indicesByteOffset;
    u32 indicesByteLength;
    u32 indicesCount;
    u32 posByteOffset;
    u32 posByteLength;

#define MESH_CENTER_AND_MIN_Y_SIZE 4
    r32 meshCenterAndMinY[MESH_CENTER_AND_MIN_Y_SIZE];

    u8 *data;
    u32 dataSize;
} RenderEntryMesh;

typedef struct
{
    u32 tileCountPerSide;
    r32 tileSideLength;
    V3 color;
    V3 originTileCenterPositionWorld;
} RenderEntryTile;

typedef struct RenderGroup
{
    u32 maxPushBufferSize;
    u32 pushBufferSize;
    u8 *pushBufferBase;
    u32 pushBufferElementCount;
    RenderGroupEntryHeader *prevHeader;
    M44 uniforms[2];
} RenderGroup;

#endif

#ifndef _ANTIQUA_RENDER_GROUP_H_
#define _ANTIQUA_RENDER_GROUP_H_

#include "types.h"
#include "antiqua_math.h"

typedef enum RenderGroupEntryType
{
    RenderGroupEntryType_RenderEntryClear,
    RenderGroupEntryType_RenderEntryPoint,
    RenderGroupEntryType_RenderEntryLine,
    RenderGroupEntryType_RenderEntryTile,
    RenderGroupEntryType_RenderEntryRect,
    RenderGroupEntryType_RenderEntryTextureDebug,
    RenderGroupEntryType_RenderEntryText,

    RenderGroupEntryType_Count,
} RenderGroupEntryType;

typedef struct RenderGroupEntryHeader
{
    RenderGroupEntryType type;
    RenderGroupEntryHeader *next;
} RenderGroupEntryHeader;

typedef struct RenderEntryClear
{
    V4 color;
} RenderEntryClear;

typedef struct RenderEntryPoint
{
    V3 position;
    V3 color;
} RenderEntryPoint;

typedef struct RenderEntryLine
{
    /* NOTE(dima): index 0 - start, index 1 - end */
    V3 color;
    V3 start;
    V3 end;
} RenderEntryLine;

typedef struct RenderEntryTile
{
    u32 tileCountPerSide;
    r32 tileSideLength;
    V3 color;
    V3 originTileCenterPositionWorld;
} RenderEntryTile;

typedef struct RenderEntryRect
{
    r32 sideLengthW;
    r32 sideLengthH;
    V3 rectCenterPositionWorld;
    V3 color;
} RenderEntryRect;

typedef struct RenderEntryTextureDebug
{
    AssetHeader *textureHeader;
    b32 needsGpuReupload;
} RenderEntryTextureDebug;

typedef struct RenderEntryText
{
    V3 color;
    GlyphMetadata *glyphMetadata;
    AssetHeader *atlasHeader;
    V2 posScreen;
    s8 *text;
    b32 needsGpuReupload;
    u8 firstGlyphCode;
} RenderEntryText;

typedef struct RenderGroup
{
    M44 uniforms[2];
    u8 *pushBufferBase;
    RenderGroupEntryHeader *prevHeader;
    AssetHeader *atlasHeader;
    u32 maxPushBufferSize;
    u32 pushBufferSize;
    u32 pushBufferElementCount;
} RenderGroup;

#endif

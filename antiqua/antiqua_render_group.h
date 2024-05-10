#ifndef _ANTIQUA_RENDER_GROUP_H_
#define _ANTIQUA_RENDER_GROUP_H_

#include "types.h"
#include "antiqua_math.h"

typedef enum
{
    RenderGroupEntryType_RenderEntryClear,
    RenderGroupEntryType_RenderEntryLine,
    RenderGroupEntryType_RenderEntryMesh
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
    /* NOTE(dima): index 0 - start, index 1 - end */
    V3 color;
    V3 start;
    V3 end;
} RenderEntryLine;

typedef struct
{
    u32 size;
    /* NOTE(dima): colors are packed together with vertices,
                   like this: [vertex color vertex color] */
    r32 *data;
} RenderEntryMesh;

typedef struct RenderGroup
{
    u32 maxPushBufferSize;
    u32 pushBufferSize;
    u8 *pushBufferBase;
    u32 pushBufferElementCount;
    RenderGroupEntryHeader *prevHeader;
    M44 uniforms[3];
} RenderGroup;

#endif

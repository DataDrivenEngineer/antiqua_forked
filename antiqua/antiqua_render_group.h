#ifndef _ANTIQUA_RENDER_GROUP_H_
#define _ANTIQUA_RENDER_GROUP_H_

#include "types.h"
#include "antiqua_math.h"

typedef enum
{
    RenderGroupEntryType_RenderEntryClear,
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
    r32 *data;
    u32 size;
} RenderEntryMesh;

typedef struct RenderGroup
{
    u32 maxPushBufferSize;
    u32 pushBufferSize;
    u8 *pushBufferBase;
    u32 pushBufferElementCount;
    M44 uniforms[3];
} RenderGroup;

#endif

#ifndef _ANTIQUA_RENDER_GROUP_H_
#define _ANTIQUA_RENDER_GROUP_H_

#include "types.h"
#include "antiqua_math.h"

typedef enum
{
    RenderGroupEntryType_RenderEntryClear,
    RenderGroupEntryType_RenderEntryBitmap,
    RenderGroupEntryType_RenderEntryRectangle,
    RenderGroupEntryType_RenderEntryCoordinateSystem,
} RenderGroupEntryType;

typedef struct
{
    RenderGroupEntryType type;
} RenderGroupEntryHeader;

typedef struct
{
    v4 color;
} RenderEntryClear;

typedef struct
{
    v4 color;
    v2 p;
    v2 dim;
} RenderEntryRectangle;

typedef struct
{
    u32 maxPushBufferSize;
    u32 pushBufferSize;
    u8 *pushBufferBase;
    u32 pushBufferElementCount;
} RenderGroup;

#endif

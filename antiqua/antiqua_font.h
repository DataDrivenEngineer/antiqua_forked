#ifndef _ANTIQUA_FONT_H_
#define _ANTIQUA_FONT_H_

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#if 0
#define STB_IMAGE_WRITE_IMPLEMENTATION
#endif
#if 0
#include "stb_image_write.h"
#endif

struct AssetHeader;

typedef struct GlyphMetadata
{
    u32 atlasRowOffset;
    u32 atlasColumnOffset;
    u32 glyphWidth;
    u32 glyphHeight;
    s32 yNegativeOffset;
    r32 advanceWidth;
    r32 leftSideBearing;
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

#endif

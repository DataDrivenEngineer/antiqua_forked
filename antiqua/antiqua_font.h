#ifndef _ANTIQUA_FONT_H_
#define _ANTIQUA_FONT_H_

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#if 0
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#endif

struct AssetHeader;

typedef struct GlyphMetadata
{
    u32 atlasRowOffset;
    u32 atlasColumnOffset;
    r32 defaultGlyphWidth;
    r32 defaultGlyphHeight;
    r32 glyphWidth;
    r32 glyphHeight;
    r32 yNegativeOffset;
    r32 advanceWidth;
    r32 leftSideBearing;
} GlyphMetadata;

typedef struct Font
{
    GlyphMetadata *glyphMetadata;
    AssetHeader *atlasHeader;
    u32 glyphCount;
    b32 needsGpuReupload;
    s32 ascent;
    s32 descent;
    s32 lineGap;
    u8 firstGlyphCode;
    u8 lastGlyphCode;
    u8 defaultFontSizePx;
} Font;

#endif

void LoadFont(GameState *gameState, GameMemory *memory)
{
    debug_ReadFileResult ttfFile;
#define FILENAME "C:/Windows/Fonts/arial.ttf"
    memory->debug_platformReadEntireFile(NULL, &ttfFile, FILENAME);
#undef FILENAME
    ASSERT(ttfFile.contentsSize != 0);

    stbtt_fontinfo font;
    stbtt_InitFont(&font, (u8 *)ttfFile.contents, stbtt_GetFontOffsetForIndex((u8 *)ttfFile.contents, 0));

    s32 ascent, descent, lineGap;
    stbtt_GetFontVMetrics(&font, &ascent, &descent, &lineGap);

    memory->debug_platformFreeFileMemory(NULL, &ttfFile);

    MemoryArena fontAtlasArena;
    u32 fontAtlasArenaSize = MB(5);
    initializeArenaFromPermanentStorage(memory, &fontAtlasArena, fontAtlasArenaSize);

    gameState->font.firstGlyphCode = 31;
    gameState->font.lastGlyphCode = 127;
    gameState->font.glyphCount = gameState->font.lastGlyphCode - gameState->font.firstGlyphCode;
    gameState->font.atlasHeader = PUSH_STRUCT(&fontAtlasArena, AssetHeader);
    gameState->font.needsGpuReupload = true;
    gameState->font.ascent = ascent;
    gameState->font.descent = descent;
    gameState->font.lineGap = lineGap;
    gameState->font.defaultFontSizePt = 16;
    AssetHeader *atlasHeader = gameState->font.atlasHeader;
    atlasHeader->pixelSizeBytes = 4;
    atlasHeader->width = 1024;
    atlasHeader->height = 1024;

    r32 fontSizePx = 1.333333f*gameState->font.defaultFontSizePt;
    r32 fontScale = stbtt_ScaleForMappingEmToPixels(&font, fontSizePx);

    MemoryArena monoBitmapArena;
    u32 sizeOfMonoBitmapArena = MB(5);
    initializeArenaFromTransientStorage(memory, &monoBitmapArena, sizeOfMonoBitmapArena);

    s32 atlasPitch = atlasHeader->width*atlasHeader->pixelSizeBytes;
    u8 *fontAtlas = PUSH_ARRAY(&fontAtlasArena, atlasPitch*atlasHeader->height, u8);

    gameState->font.glyphMetadata = PUSH_ARRAY(&fontAtlasArena, gameState->font.glyphCount, GlyphMetadata);

    s32 offsetToNextAtlasRow = 0, currentAtlasRow = 0, currentAtlasColumn = 0;

    for (u32 glyphASCIICode = gameState->font.firstGlyphCode;
         glyphASCIICode < gameState->font.lastGlyphCode;
         ++glyphASCIICode)
    {
        s32 ix0, ix1, iy0, iy1;
        stbtt_GetCodepointBitmapBox(&font, glyphASCIICode, fontScale, fontScale, &ix0, &iy0, &ix1, &iy1);
        s32 width = ix1 - ix0;
        s32 height = iy1 - iy0;

        s32 advanceWidth, leftSideBearing;
        stbtt_GetCodepointHMetrics(&font, glyphASCIICode, &advanceWidth, &leftSideBearing);

        u8 *monoBitmap = PUSH_ARRAY(&monoBitmapArena, width*height, u8);

        stbtt_MakeCodepointBitmap(&font, monoBitmap, width, height, width, fontScale, fontScale, glyphASCIICode);

        u8 *srcBitmap = monoBitmap;

        // Add 1px (each pixel has 4 subpixels) offset to every glyph
        u8 glyphLeftPaddingPx = 4;
        u8 glyphRightPaddingPx = 0;
        currentAtlasColumn += glyphLeftPaddingPx;

        if (atlasPitch - currentAtlasColumn < (s32)atlasHeader->pixelSizeBytes*width)
        {
            currentAtlasRow += offsetToNextAtlasRow;
            currentAtlasColumn = 0 + glyphLeftPaddingPx;

            offsetToNextAtlasRow = 0;
        }

        ASSERT(atlasPitch - currentAtlasColumn >= (s32)atlasHeader->pixelSizeBytes*width + glyphRightPaddingPx);
        ASSERT((s32)atlasHeader->height - currentAtlasRow >= height);

        offsetToNextAtlasRow = height > offsetToNextAtlasRow ? height : offsetToNextAtlasRow;

        u8 *dstAtlas = fontAtlas + atlasPitch*currentAtlasRow + currentAtlasColumn;
        ASSERT(dstAtlas - fontAtlas < fontAtlasArenaSize);

        for (s32 y = 0; y < height; y++)
        {
            u32 *dst = (u32 *)dstAtlas;
            for (s32 x = 0; x < width; x++)
            {
                u8 alpha = *srcBitmap++;
                *dst++ = ((alpha << 24) |
                          (alpha << 16) |
                          (alpha <<  8) |
                          (alpha <<  0));
            }

            dstAtlas += atlasPitch;
        }

        GlyphMetadata *currentGlyphMetadata = gameState->font.glyphMetadata + (glyphASCIICode - gameState->font.firstGlyphCode);

        currentGlyphMetadata->atlasRowOffset     = currentAtlasRow;
        currentGlyphMetadata->atlasColumnOffset  = currentAtlasColumn;
        currentGlyphMetadata->defaultGlyphWidth  = (r32)width + 1;
        currentGlyphMetadata->defaultGlyphHeight = (r32)height;
        currentGlyphMetadata->glyphWidth         = (r32)width + 1;
        currentGlyphMetadata->glyphHeight        = (r32)height;
        currentGlyphMetadata->yNegativeOffset    = (r32)iy1;
        currentGlyphMetadata->advanceWidth       = advanceWidth*fontScale;
        currentGlyphMetadata->leftSideBearing    = leftSideBearing*fontScale;

        currentAtlasColumn += atlasHeader->pixelSizeBytes*width + glyphRightPaddingPx;
    }

    deallocateArenaFromTransientStorage(memory, sizeOfMonoBitmapArena);

#if 0
    stbi_write_bmp("tmp\\fontAtlas.bmp", 1024, 1024, 4, fontAtlas);
#endif
}

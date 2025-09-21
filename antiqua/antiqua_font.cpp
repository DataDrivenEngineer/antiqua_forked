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

    MemoryArena fontArena;
    initializeArenaFromPermanentStorage(memory, &fontArena, MB(5));

    gameState->font.firstGlyphCode = 31;
    gameState->font.lastGlyphCode = 127;
    gameState->font.glyphCount = gameState->font.lastGlyphCode - gameState->font.firstGlyphCode;
    gameState->font.atlasHeader = PUSH_STRUCT(&fontArena, AssetHeader);
    gameState->font.needsGpuReupload = true;
    gameState->font.ascent = ascent;
    gameState->font.descent = descent;
    gameState->font.lineGap = lineGap;
    gameState->font.defaultFontSizePx = 128;
    AssetHeader *atlasHeader = gameState->font.atlasHeader;
    atlasHeader->pixelSizeBytes = 4;
    atlasHeader->width = 1024;
    atlasHeader->height = 1024;

    // Default scale, actual scaling will be applied by renderer depending on the px size passed into API
    r32 fontScale = stbtt_ScaleForPixelHeight(&font, (r32)gameState->font.defaultFontSizePx);

    MemoryArena monoBitmapArena;
    u32 sizeOfMonoBitmapArena = MB(5);
    initializeArenaFromTransientStorage(memory, &monoBitmapArena, sizeOfMonoBitmapArena);

    s32 atlasPitch = atlasHeader->width*atlasHeader->pixelSizeBytes;
    u8 *fontAtlas = PUSH_ARRAY(&fontArena, atlasPitch*atlasHeader->height, u8);

    gameState->font.glyphMetadata = PUSH_ARRAY(&fontArena, gameState->font.glyphCount, GlyphMetadata);

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

        if (atlasPitch - currentAtlasColumn < (s32)atlasHeader->pixelSizeBytes*width)
        {
            currentAtlasRow += offsetToNextAtlasRow;
            currentAtlasColumn = 0;

            offsetToNextAtlasRow = 0;
        }

        ASSERT(atlasPitch - currentAtlasColumn >= (s32)atlasHeader->pixelSizeBytes*width);
        ASSERT((s32)atlasHeader->height - currentAtlasRow >= height);

        offsetToNextAtlasRow = height > offsetToNextAtlasRow ? height : offsetToNextAtlasRow;

        u8 *dstAtlas = fontAtlas + atlasPitch*currentAtlasRow + currentAtlasColumn;

        for (s32 y = 0; y < height; y++)
        {
            u32 *dst = (u32 *)dstAtlas;
            for (s32 x = 0; x < width; x++)
            {
                u8 alpha = *srcBitmap++;
//TODO: figure out how to resolve color dynamically in shader. Right now below just hardcodes white.
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
        currentGlyphMetadata->defaultGlyphWidth  = (r32)width;
        currentGlyphMetadata->defaultGlyphHeight = (r32)height;
        currentGlyphMetadata->glyphWidth         = (r32)width;
        currentGlyphMetadata->glyphHeight        = (r32)height;
        currentGlyphMetadata->yNegativeOffset    = (r32)iy1;
        currentGlyphMetadata->advanceWidth       = advanceWidth*fontScale;
        currentGlyphMetadata->leftSideBearing    = leftSideBearing*fontScale;

        currentAtlasColumn += atlasHeader->pixelSizeBytes*width;
    }

    deallocateArenaFromTransientStorage(memory, sizeOfMonoBitmapArena);

#if 0
    stbi_write_bmp("tmp\\fontAtlas.bmp", 1024, 1024, 4, fontAtlas);
#endif
}

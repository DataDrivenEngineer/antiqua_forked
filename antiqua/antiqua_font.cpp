internal void
LoadFont(GameState *gameState, GameMemory *memory)
{
    debug_ReadFileResult ttfFile;
#define FILENAME "C:/Windows/Fonts/arial.ttf"
    memory->debug_platformReadEntireFile(NULL, &ttfFile, FILENAME);
#undef FILENAME
    ASSERT(ttfFile.contentsSize != 0);

    stbtt_fontinfo font;
    stbtt_InitFont(&font, (u8 *)ttfFile.contents, stbtt_GetFontOffsetForIndex((u8 *)ttfFile.contents, 0));
    r32 fontScale = stbtt_ScaleForPixelHeight(&font, 128.0f);

    memory->debug_platformFreeFileMemory(NULL, &ttfFile);

    MemoryArena fontArena;
    initializeArenaFromPermanentStorage(memory, &fontArena, MB(5));

    gameState->font.firstGlyphCode = 32;
    gameState->font.lastGlyphCode = 127;
    gameState->font.glyphCount = gameState->font.lastGlyphCode - gameState->font.firstGlyphCode;
    gameState->font.atlasHeader = PUSH_STRUCT(&fontArena, AssetHeader);
    gameState->font.needsGpuReupload = true;
    AssetHeader *atlasHeader = gameState->font.atlasHeader;
    atlasHeader->pixelSizeBytes = 4;
    atlasHeader->width = 1024;
    atlasHeader->height = 1024;

    MemoryArena monoBitmapArena;
    u32 sizeOfMonoBitmapArena = MB(5);
    initializeArenaFromTransientStorage(memory, &monoBitmapArena, sizeOfMonoBitmapArena);

#define ATLAS_WIDTH (s32)atlasHeader->width
#define ATLAS_HEIGHT (s32)atlasHeader->height
#define ATLAS_PITCH (s32)atlasHeader->pixelSizeBytes*ATLAS_WIDTH
    u8 *fontAtlas = PUSH_ARRAY(&fontArena, ATLAS_PITCH*ATLAS_HEIGHT, u8);

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

        if (ATLAS_PITCH - currentAtlasColumn < (s32)atlasHeader->pixelSizeBytes*width)
        {
            currentAtlasRow += offsetToNextAtlasRow;
            currentAtlasColumn = 0;

            offsetToNextAtlasRow = 0;
        }

        ASSERT(ATLAS_PITCH - currentAtlasColumn >= (s32)atlasHeader->pixelSizeBytes*width);
        ASSERT(ATLAS_HEIGHT - currentAtlasRow >= height);

        offsetToNextAtlasRow = height > offsetToNextAtlasRow ? height : offsetToNextAtlasRow;

        u8 *dstAtlas = fontAtlas + ATLAS_PITCH*currentAtlasRow + currentAtlasColumn;

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

            dstAtlas += ATLAS_PITCH;
        }

        GlyphMetadata *currentGlyphMetadata = gameState->font.glyphMetadata + (glyphASCIICode - gameState->font.firstGlyphCode);

        currentGlyphMetadata->atlasRowOffset    = currentAtlasRow;
        currentGlyphMetadata->atlasColumnOffset = currentAtlasColumn;
        currentGlyphMetadata->glyphWidth        = width;
        currentGlyphMetadata->glyphHeight       = height;
        currentGlyphMetadata->yNegativeOffset   = iy1;
        currentGlyphMetadata->advanceWidth      = advanceWidth*fontScale;
        currentGlyphMetadata->leftSideBearing   = leftSideBearing*fontScale;

        currentAtlasColumn += atlasHeader->pixelSizeBytes*width;
    }
#undef ATLAS_PITCH
#undef ATLAS_WIDTH
#undef ATLAS_HEIGHT

    deallocateArenaFromTransientStorage(memory, sizeOfMonoBitmapArena);

#if 0
    stbi_write_bmp("tmp\\fontAtlas.bmp", 1024, 1024, 4, fontAtlas);
#endif
}


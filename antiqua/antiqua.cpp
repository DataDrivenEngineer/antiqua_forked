#include "antiqua.h"
#include "stdio.h"
#include "antiqua_intrinsics.h"
#include "antiqua_render_group.h"

#define STB_TRUETYPE_IMPLEMENTATION
#include "stb_truetype.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#include "antiqua_render_group.cpp"

#define TEXTURE_DEBUG_MODE 0

static void CastRayToClickPositionOnTilemap(GameState *gameState,
                                            r32 drawableWidthWithoutScaleFactor,
                                            r32 drawableHeightWithoutScaleFactor)
{
    // NOTE(dima): convert mouse posistion to world space
    r32 mouseX = gameState->mousePos[0];
    r32 mouseY = gameState->mousePos[1];

    r32 xScale = 2.0f / drawableWidthWithoutScaleFactor;
    r32 clipX = mouseX * xScale - 1.0f;
    r32 yScale = 2.0f / drawableHeightWithoutScaleFactor;
    r32 clipY = -(mouseY * yScale - 1.0f);

    V4 clipPos = v4(clipX, clipY, 0.0f, 1.0f);
    M44 inverseProjectionMatrix = inverse(&gameState->projectionMatrix);
    M44 inverseViewMatrix = inverse(&gameState->viewMatrix);
    V4 mousePosNearPlaneCamera = inverseProjectionMatrix * clipPos;
    mousePosNearPlaneCamera.z = gameState->nearPlane + 0.001f;

    V4 cameraDirectonPosVectorStartCamera = mousePosNearPlaneCamera;
    cameraDirectonPosVectorStartCamera.z = gameState->nearPlane + 0.001f;
    V4 cameraDirectonPosVectorEndCamera = mousePosNearPlaneCamera;
    cameraDirectonPosVectorEndCamera.z = cameraDirectonPosVectorStartCamera.z + 1.0f;
    V4 cameraDirectonPosVectorStartWorld = inverseViewMatrix * cameraDirectonPosVectorStartCamera;
    V4 cameraDirectonPosVectorEndWorld = inverseViewMatrix * cameraDirectonPosVectorEndCamera;
    gameState->cameraDirectonPosVectorStartWorld = v3(cameraDirectonPosVectorStartWorld);
    gameState->cameraDirectonPosVectorEndWorld = v3(cameraDirectonPosVectorEndWorld);

    V4 mouseDirectionVectorCamera = mousePosNearPlaneCamera;
    mouseDirectionVectorCamera.z = 1.0f;
    mouseDirectionVectorCamera.w = 0.0f;
    gameState->mouseDirectionVectorWorld = v3(inverseViewMatrix * mouseDirectionVectorCamera);

    V3 n = gameState->n;
    V3 v = gameState->v;
    V3 u = gameState->u;

    V3 mouseDirectionVectorProjectionUNWorld = (dot(v, gameState->mouseDirectionVectorWorld)
                                                / squareLength(v)) * v;
    gameState->mouseDirectionVectorHorizontalWorld = gameState->mouseDirectionVectorWorld - mouseDirectionVectorProjectionUNWorld;
    normalize(&gameState->mouseDirectionVectorHorizontalWorld);
    r32 cosNRHorizontal = dot(n, gameState->mouseDirectionVectorHorizontalWorld);
    r32 horizontalAngleDegrees = DEGREES(arccosine(cosNRHorizontal));
    /* NOTE(dima): this is required, because cos(a) = cos(-a),
       but we still want to differentiate between negative and positive angles */
    if (mousePosNearPlaneCamera.x < 0)
    {
        horizontalAngleDegrees = 360.0f - horizontalAngleDegrees;
    }

    V3 mouseDirectionVectorProjectionNVWorld = (dot(u, gameState->mouseDirectionVectorWorld)
                                                / squareLength(u)) * u;
    gameState->mouseDirectionVectorVerticalWorld = gameState->mouseDirectionVectorWorld - mouseDirectionVectorProjectionNVWorld;
    normalize(&gameState->mouseDirectionVectorVerticalWorld);
    r32 cosNRVertical = dot(n, gameState->mouseDirectionVectorVerticalWorld);
    r32 verticalAngleDegrees = -DEGREES(arccosine(cosNRVertical));
    if (mousePosNearPlaneCamera.y < 0)
    {
        verticalAngleDegrees = -verticalAngleDegrees;
    }
    
    rotate(&n, v, horizontalAngleDegrees);
    u = cross(v, n);
    normalize(&u);
    rotate(&n, u, verticalAngleDegrees);
    normalize(&n);

    V3 tilemapOriginToMousePositionVectorWorld = gameState->tilemapOriginPositionWorld
                                                 - gameState->cameraDirectonPosVectorStartWorld;
    V3 tileNormalWorld = v3(0.0f, 1.0f, 0.0f);
    r32 lengthOfVectorToReachTilemap = dot(tileNormalWorld, tilemapOriginToMousePositionVectorWorld)
                                       / dot(n, tileNormalWorld);

    gameState->cameraDirectonPosVectorEndWorld = gameState->cameraDirectonPosVectorStartWorld
                                                 + n * lengthOfVectorToReachTilemap;
    r32 halfTilemapCoordinateOffset = (gameState->tileCountPerSide / 2) * gameState->tileSideLength;
    if (gameState->cameraDirectonPosVectorEndWorld.x >= (gameState->tilemapOriginPositionWorld.x - halfTilemapCoordinateOffset)
        && gameState->cameraDirectonPosVectorEndWorld.x <= (gameState->tilemapOriginPositionWorld.x + halfTilemapCoordinateOffset)
        && gameState->cameraDirectonPosVectorEndWorld.z >= (gameState->tilemapOriginPositionWorld.z - halfTilemapCoordinateOffset)
        && gameState->cameraDirectonPosVectorEndWorld.z <= (gameState->tilemapOriginPositionWorld.z + halfTilemapCoordinateOffset))
    {
        gameState->isClickPositionInsideTilemap = true;
    }
    else
    {
        gameState->isClickPositionInsideTilemap = false;
    }
}

internal void MoveEntity(GameState *gameState,
                       Entity *testEntity,
                       u32 testEntityIndex,
                       r32 deltaTimeSec,
                       V3 acceleration,
                       V3 posDelta)
{
#if 0
    r32 static deltaTimeSecConst = 0;
    if (!deltaTimeSecConst && deltaTimeSec < 1.0f)
    {
        deltaTimeSecConst = deltaTimeSec;
    }
    deltaTimeSec = deltaTimeSecConst;
#endif

    testEntity->dPos = acceleration * deltaTimeSec + testEntity->dPos;

    // NOTE(dima): collision detection and response using AABB

    V3 posWorld = {0};

    r32 tRemaining = 1.0f;
    for (u32 iteration = 0;
         iteration < 4 && tRemaining > 0.0f;
         ++iteration)
    {
        r32 tMin = 1.0f;
        V3 lineNormalVector = {0};

        r32 tEpsilon = 0.001f;

        for (u32 entityIndex = 0;
             entityIndex < gameState->entityCount;
             ++entityIndex)
        {
            if (testEntityIndex == entityIndex)
            {
                continue;
            }

            Entity *entity = gameState->entities + entityIndex;

            posWorld = v3(testEntity->posWorld.x,
                          0.0f,
                          testEntity->posWorld.z);

            Rect entityRegularAABB;
            entityRegularAABB.diameterW = gameState->tileSideLength;
            entityRegularAABB.diameterH = gameState->tileSideLength;
            entityRegularAABB.diameterW *= entity->scaleFactor.x;
            entityRegularAABB.diameterH *= entity->scaleFactor.z;

            Rect testEntityRegularAABB;
            testEntityRegularAABB.diameterW = gameState->tileSideLength;
            testEntityRegularAABB.diameterH = gameState->tileSideLength;
            testEntityRegularAABB.diameterW *= testEntity->scaleFactor.x;
            testEntityRegularAABB.diameterH *= testEntity->scaleFactor.z;

            Rect aabbAfterMinkowskiSum;
            aabbAfterMinkowskiSum.diameterW =
                testEntityRegularAABB.diameterW + entityRegularAABB.diameterW;
            aabbAfterMinkowskiSum.diameterH =
                testEntityRegularAABB.diameterH + entityRegularAABB.diameterH;

            Rect box = aabbAfterMinkowskiSum;
            V3 topLeft = v3(-0.5f*box.diameterW,
                            0.0f,
                            0.5f*box.diameterH);
            V3 topRight = v3(0.5f*box.diameterW,
                             0.0f,
                             0.5f*box.diameterH);
            V3 bottomLeft = v3(-0.5f*box.diameterW,
                               0.0f,
                               -0.5f*box.diameterH);
            V3 bottomRight = v3(0.5f*box.diameterW,
                                0.0f,
                                -0.5f*box.diameterH);

            /* NOTE(dima): indices 0,1 - left bottom to right bottom
                           indices 2,3 - left top to left bottom
                           indices 4,5 - right bottom to right top
                           indices 6,7 - right top to left top */
            V3 lines[4*2];
            // NOTE(dima): must preserve same winding order for consistent normal calculation!
            lines[0] = bottomRight;
            lines[1] = bottomLeft;
            lines[2] = bottomLeft;
            lines[3] = topLeft;
            lines[4] = topRight;
            lines[5] = bottomRight;
            lines[6] = topLeft;
            lines[7] = topRight;

            V3 ro = posWorld - entity->posWorld;
            ro.y = 0.0f;

            V3 lineNormalVectors[2];
            u32 lineNormalVectorsSize = 0;

            for (u32 lineIndex = 0;
                 lineIndex < 7;
                 lineIndex += 2)
            {
                V3 qo = lines[lineIndex];
                V3 q1 = lines[lineIndex + 1];
                V3 d = posDelta;
                V3 s = q1 - qo;
                r32 denominator = crossXZ(d, s);

                if (denominator != 0.0f)
                {
                    r32 numeratorT = crossXZ(qo - ro, s);
                    r32 t = numeratorT / denominator;
                    r32 numeratorU = crossXZ(qo - ro, d);
                    r32 u = numeratorU / denominator;

                    b32 collide = (t > 0.0f || numeratorT == 0)
                                  && (t < 1.0f || numeratorT == denominator)
                                  && (u > 0.0f || numeratorU == 0)
                                  && (u < 1.0f || numeratorU == denominator);

                    r32 dx = q1.x - qo.x;
                    r32 dz = q1.z - qo.z;

                    if (collide)
                    {
                        lineNormalVectors[lineNormalVectorsSize] = v3(-dz, 0.0f, dx);
                        lineNormalVectorsSize++;
                        if (t < tMin)
                        {
                            lineNormalVector.x = -dz;
                            lineNormalVector.z = dx;

                            if (!gameState->prevTimeOfCollision)
                            {
                               gameState->prevTimeOfCollision = t;
                            }
                            else
                            {
                                if (t > 0.0005f
                                    && t > gameState->prevTimeOfCollision)
                                {
                                    t = gameState->prevTimeOfCollision;
                                }
                                else
                                {
                                    gameState->prevTimeOfCollision = t;
                                }
                            }

                            tMin = t;

                            V3 collisionPointPositionWorld = (posWorld
                                                              + tMin * posDelta);
                            gameState->collisionPointDebug = v3(collisionPointPositionWorld.x,
                                                             0.0f,
                                                             collisionPointPositionWorld.z);
                        }
                    }
                }
            }

            ASSERT(lineNormalVectorsSize <= 2);

            if (lineNormalVectorsSize == 2)
            {
                for (u32 idx = 0;
                     idx < lineNormalVectorsSize;
                     ++idx)
                {
                    V3 currNormal = lineNormalVectors[idx];
                    if (currNormal.x != lineNormalVector.x
                        || currNormal.z != lineNormalVector.z)
                    {
                        lineNormalVector = lineNormalVector + currNormal;
                    }
                }

                // NOTE(dima): cheap normalization (no deletion)
                if (lineNormalVector.x < 0.0f) {

                    lineNormalVector.x = -1.0f;
                }
                else if (lineNormalVector.x > 0.0f)
                {
                    lineNormalVector.x = 1.0f;
                }
                if (lineNormalVector.z < 0.0f)
                {
                    lineNormalVector.z = -1.0f;
                }
                else if (lineNormalVector.z > 0.0f)
                {
                    lineNormalVector.z = 1.0f;
                }

                V3 dPosPlusNormal = testEntity->dPos + lineNormalVector;
                V3 dPosRotatedBy90Degrees = testEntity->dPos;
                rotate(&dPosRotatedBy90Degrees,
                       v3(0.0f, 1.0f, 0.0f),
                       90.0f);
                b32 dPosMovingClockwise = dot(dPosRotatedBy90Degrees, dPosPlusNormal) < 0;
#define NORMAL_ROTATION_ANGLE_DEGREES 51
                if (dPosMovingClockwise)
                {
                    rotate(&lineNormalVector,
                           v3(0.0f, 1.0f, 0.0f),
                           -NORMAL_ROTATION_ANGLE_DEGREES);
                }
                else
                {
                    rotate(&lineNormalVector,
                           v3(0.0f, 1.0f, 0.0f),
                           NORMAL_ROTATION_ANGLE_DEGREES);
                }
            }

            normalize(&lineNormalVector);
        }

        if (lineNormalVector.x != 0.0f
            || lineNormalVector.z != 0.0f)
        {
            gameState->lineNormalVector = lineNormalVector;
        }

        posWorld = posWorld + posDelta*maximum(0.0f, tMin - tEpsilon);
        posDelta = posDelta - 1*dot(posDelta, lineNormalVector) * lineNormalVector;

        testEntity->dPos = testEntity->dPos
                           - (1*dot(testEntity->dPos, lineNormalVector)*lineNormalVector);

        tRemaining -= tRemaining*tMin;

        testEntity->posWorld.x = posWorld.x;
        testEntity->posWorld.z = posWorld.z;
    }
}

static void UpdatePlayer(GameState *gameState,
                         Entity *entity,
                         u32 entityIndex,
                         r32 deltaTimeSec,
                         V3 playerAcceleration)
{
    normalize(&playerAcceleration);
    playerAcceleration *= gameState->playerSpeed;
    playerAcceleration = playerAcceleration + -3.0f * entity->dPos;
    V3 posDelta = (0.5f * playerAcceleration * square(deltaTimeSec) + entity->dPos * deltaTimeSec);

    MoveEntity(gameState, entity, entityIndex, deltaTimeSec, playerAcceleration, posDelta);
}

static void UpdateEnemy(GameState *gameState,
                        Entity *entity,
                        u32 entityIndex,
                        r32 deltaTimeSec)
{
    static r32 counter = 0.0f;
    counter += 0.01f;
    if (counter > 10000.0f)
    {
        counter = 0.0f;
    }

    V3 acceleration = v3(cosine(counter),
                         0.0f,
                         sine((counter)));

    normalize(&acceleration);
    acceleration *= 0.5f*gameState->playerSpeed;
    acceleration = acceleration + -3.0f * entity->dPos;
    if (deltaTimeSec > 100.0f)
    {
        deltaTimeSec = 0.0f;
    }
    V3 posDelta = 0.5f * acceleration * square(deltaTimeSec) + entity->dPos * deltaTimeSec;

    MoveEntity(gameState, entity, entityIndex, deltaTimeSec, acceleration, posDelta);
}

static void UpdateStaticEntity(GameState *gameState,
                               Entity *entity)
{
}

#if 0
internal void MakeNothingsTest(GameMemory *Memory)
{
    debug_ReadFileResult TTFFile;
#define FILENAME "C:/Windows/Fonts/arial.ttf"
    Memory->debug_platformReadEntireFile(NULL, &TTFFile, FILENAME);
#undef FILENAME
    ASSERT(TTFFile.contentsSize != 0);

    stbtt_fontinfo Font;
    stbtt_InitFont(&Font, (u8 *)TTFFile.contents, stbtt_GetFontOffsetForIndex((u8 *)TTFFile.contents, 0));
    s32 Width, Height, XOffset, YOffset;
    u8 *MonoBitmap = stbtt_GetCodepointBitmap(&Font, 0, stbtt_ScaleForPixelHeight(&Font, 128.0f), 'n', &Width, &Height, &XOffset, &YOffset);

    stbtt_FreeBitmap(MonoBitmap, NULL);
}
#endif

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

        currentAtlasColumn += atlasHeader->pixelSizeBytes*width;
    }
#undef ATLAS_PITCH
#undef ATLAS_WIDTH
#undef ATLAS_HEIGHT

    deallocateArenaFromTransientStorage(memory, sizeOfMonoBitmapArena);

    stbi_write_bmp("tmp\\fontAtlas.bmp", 1024, 1024, 4, fontAtlas);
}

#if !XCODE_BUILD && !COMPILER_MSVC
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
    /* TODO(dima):
    */

    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
    GameState *gameState = (GameState *) memory->permanentStorage;

    // NOTE(dima): Reset any state here
    // TODO: should we refactor into separate beginRender() function?
    {
        memory->usedTransientStorage = 0;
    }

    MemoryArena renderGroupArena;
    u32 renderGroupArenaSize = KB(256);
    initializeArenaFromTransientStorage(memory,
                                        &renderGroupArena,
                                        renderGroupArenaSize);

    MemoryArena scratchArena;
    u32 scratchArenaSize = KB(256);
    initializeArenaFromTransientStorage(memory,
                                        &scratchArena,
                                        scratchArenaSize);

    ASSERT(&gcInput->terminator - &gcInput->buttons[0] == ARRAY_COUNT(gcInput->buttons));
    if (!memory->isInitialized)
    {
        // do initialization here as needed

        memory->usedPermanentStorage += sizeof(GameState);

        LoadFont(gameState, memory);

        gameState->nearPlane = 1.0f;
        gameState->farPlane = 100.0f;
        gameState->fov = 45.0f;

        gameState->playerSpeed = 17.0f;

        gameState->cameraMinDistance = 5.0f;
        gameState->cameraMaxDistance = 20.0f;
        gameState->cameraCurrDistance = gameState->cameraMinDistance;

        gameState->cameraRotationSpeed = 0.3f;
        gameState->cameraMovementSpeed = 100.0f;

        gameState->v = v3(0.0f, 1.0f, 0.0f);
        gameState->n = v3(0.0f, 0.0f, 1.0f);
        gameState->u = cross(gameState->n, gameState->v);

        gameState->mousePos[0] = 500;
        gameState->mousePos[1] = 350;

        gameState->tilemapOriginPositionWorld = v3(0.0f, 0.0f, 0.0f);
        gameState->tileCountPerSide = 64;
        gameState->tileSideLength = 1.0f;

        { 
            // NOTE(dima): Below is isometric camera setup
            gameState->cameraIsometricVerticalAxisRotationAngleDegrees = 45.0f;
            gameState->cameraIsometricHorizontalAxisRotationAngleDegrees = 35.23f;//45.0f;
            gameState->isometricV = gameState->v;
            gameState->isometricN = gameState->n;
            gameState->isometricU = gameState->u;

            // NOTE(dima): rotate around vertical axis by 45 degrees
            rotate(&gameState->isometricN,
                   gameState->isometricV,
                   gameState->cameraIsometricVerticalAxisRotationAngleDegrees);

            // NOTE(dima): rotate around horizontal axis by 45 degrees
            gameState->isometricU = cross(gameState->isometricV, gameState->isometricN);
            normalize(&gameState->isometricU);
            rotate(&gameState->isometricN,
                   gameState->isometricU,
                   gameState->cameraIsometricHorizontalAxisRotationAngleDegrees);

            gameState->isometricV = cross(gameState->isometricN, gameState->isometricU);
            normalize(&gameState->isometricV);
        }

        // NOTE(dima): below is code to initially position entities in the world
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Moving | EntityFlags_PlayerControlled);
            newEntity->posWorld = v3(0.0f, 0.0f, 0.0f);
            newEntity->scaleFactor = v3(1.0f, 1.0f, 1.0f);
            gameState->entityCount++;
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Static);
            newEntity->posWorld = v3(5.0f, 0.0f, 5.0f);
            newEntity->scaleFactor = v3(20.0f, 1.0f, 2.0f);
            gameState->entityCount++;
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Moving | EntityFlags_Enemy);
            newEntity->posWorld = v3(6.5f, 0.0f, 7.5f);
            newEntity->scaleFactor = v3(1.0f, 2.0f, 1.0f);
            gameState->entityCount++;
        }

        gameState->cameraFollowingEntityIndex = 0;

        memory->isInitialized = true;
    }

    if (gcInput->debugCmdC.endedDown)
    {
        gameState->debugCameraEnabled = !gameState->debugCameraEnabled;
    }

    RenderGroup renderGroup = {0};
    renderGroup.maxPushBufferSize = renderGroupArenaSize;
    renderGroup.pushBufferBase = renderGroupArena.base;
    renderGroup.atlasHeader = gameState->font.atlasHeader;

    pushRenderEntryClear(&renderGroupArena,
                         &renderGroup,
                         v3(0.3f, 0.3f, 0.3f));
    pushRenderEntryTile(&renderGroupArena,
                         &renderGroup,
                         gameState->tileCountPerSide,
                         gameState->tileSideLength,
                         gameState->tilemapOriginPositionWorld,
                         v3(1.0f, 1.0f, 1.0f));

#if ANTIQUA_INTERNAL
    // NOTE(dima): for visualizing purposes only
    gameState->collisionPointDebug = {0};
#endif

    if (gameState->debugCameraEnabled)
    {
        if (gcInput->right.endedDown)
        {
            V3 u = cross(gameState->v, gameState->n);
            normalize(&u);
            gameState->cameraPosWorld = gameState->cameraPosWorld
                                        + gameState->cameraMovementSpeed * deltaTimeSec * u;
        }

        if (gcInput->left.endedDown)
        {
            V3 u = cross(gameState->n, gameState->v);
            normalize(&u);
            gameState->cameraPosWorld = gameState->cameraPosWorld
                                        + gameState->cameraMovementSpeed * deltaTimeSec * u;
        }

        if (gcInput->up.endedDown)
        {
            gameState->cameraPosWorld = gameState->cameraPosWorld
                                        + gameState->cameraMovementSpeed * deltaTimeSec * gameState->n;
        }

        if (gcInput->down.endedDown)
        {
            gameState->cameraPosWorld = gameState->cameraPosWorld
                                        - gameState->cameraMovementSpeed * deltaTimeSec * gameState->n;
        }

        if ((gcInput->mouseX != gameState->mousePos[0] || gcInput->mouseY != gameState->mousePos[1]))
        {
            V3 v = v3(0.0f, 1.0f, 0.0f);
            V3 n = v3(0.0f, 0.0f, 1.0f);
            V3 u = cross(n, v);

            r32 horizontalOffset = gcInput->mouseX - gameState->mousePos[0];
            r32 verticalOffset = gcInput->mouseY - gameState->mousePos[1];

            gameState->cameraVerticalAngle += gameState->cameraRotationSpeed * verticalOffset; 
            gameState->cameraHorizontalAngle += gameState->cameraRotationSpeed * horizontalOffset;

            // NOTE(dima): rotate by horizontal angle around vertical axis
            rotate(&n, v, gameState->cameraHorizontalAngle);

            // NOTE(dima): rotate by vertical angle around horizontal axis
            u = cross(v, n);
            normalize(&u);
            rotate(&n, u, gameState->cameraVerticalAngle);

            v = cross(n, u);
            normalize(&v);

            gameState->n = n;
            gameState->v = v;
            gameState->u = u;

            gameState->mousePos[0] = gcInput->mouseX;
            gameState->mousePos[1] = gcInput->mouseY;
        }

        for (s32 entityIndex = gameState->entityCount - 1;
             entityIndex >= 0;
             --entityIndex)
        {
            Entity *entity = gameState->entities + entityIndex;

            pushRenderEntryRect(&renderGroupArena,
                                &renderGroup,
                                entity->posWorld,
                                entity->scaleFactor.x * gameState->tileSideLength,
                                entity->scaleFactor.z * gameState->tileSideLength);
        }
    }
    else
    {
        Entity *cameraFollowingEntity = gameState->entities + gameState->cameraFollowingEntityIndex;

        V3 playerAcceleration = {};

        if (gcInput->up.endedDown)
        {
            playerAcceleration.x = gameState->v.x;
            playerAcceleration.z = gameState->v.z;
        }

        if (gcInput->down.endedDown)
        {
            playerAcceleration.x = -gameState->v.x;
            playerAcceleration.z = -gameState->v.z;
        }

        if (gcInput->right.endedDown)
        {
            V3 uWithoutY = cross(gameState->v, gameState->n);
            uWithoutY.y = 0.0f;
            V3 vWithoutY = v3(gameState->v.x, 0.0f, gameState->v.z);

            if (gcInput->up.endedDown)
            {
                normalize(&vWithoutY);
                normalize(&uWithoutY);
                V3 diagonalDirectionVector = vWithoutY + uWithoutY;
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else if (gcInput->down.endedDown)
            {
                normalize(&vWithoutY);
                normalize(&uWithoutY);
                V3 diagonalDirectionVector = -vWithoutY + uWithoutY;
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else
            {
                playerAcceleration.x = uWithoutY.x;
                playerAcceleration.z = uWithoutY.z;
            }
        }

        if (gcInput->left.endedDown)
        {
            V3 uWithoutY = cross(gameState->n, gameState->v);
            uWithoutY.y = 0.0f;
            V3 vWithoutY = v3(gameState->v.x, 0.0f, gameState->v.z);

            if (gcInput->up.endedDown)
            {
                normalize(&vWithoutY);
                normalize(&uWithoutY);
                V3 diagonalDirectionVector = vWithoutY + uWithoutY;
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else if (gcInput->down.endedDown)
            {
                normalize(&vWithoutY);
                normalize(&uWithoutY);
                V3 diagonalDirectionVector = -vWithoutY + uWithoutY;
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else
            {
                playerAcceleration.x = uWithoutY.x;
                playerAcceleration.z = uWithoutY.z;
            }
        }

        for (s32 entityIndex = gameState->entityCount - 1;
             entityIndex >= 0;
             --entityIndex)
        {
            Entity *entity = gameState->entities + entityIndex;
            EntityFlags flags = entity->flags;
            if ((flags & EntityFlags_PlayerControlled) == EntityFlags_PlayerControlled)
            {
                UpdatePlayer(gameState,
                             entity,
                             entityIndex,
                             deltaTimeSec,
                             playerAcceleration);
            }
            else if ((flags & EntityFlags_Enemy) == EntityFlags_Enemy)
            {
#if 1
                UpdateEnemy(gameState,
                            entity,
                            entityIndex,
                            deltaTimeSec);
#endif
            }
            else if ((flags & EntityFlags_Static) == EntityFlags_Static)
            {
                /* TODO(dima): this only calculates AABB!
                               DON'T do it every frame. Do it only once at loading! */
                UpdateStaticEntity(gameState,
                                   entity);
            }
            else
            {
                INVALID_CODE_PATH;
            }

            pushRenderEntryRect(&renderGroupArena,
                                &renderGroup,
                                entity->posWorld,
                                entity->scaleFactor.x * gameState->tileSideLength,
                                entity->scaleFactor.z * gameState->tileSideLength);
        }

        // NOTE(dima): set up camera to look at entity it follows
        {
            V3 cameraAcceleration = {};
            if (gcInput->scrollingDeltaY < 0)
            {
                cameraAcceleration = -gameState->n;
            }
            else if (gcInput->scrollingDeltaY > 0)
            {
                cameraAcceleration = gameState->n;
            }
            cameraAcceleration *= gameState->cameraMovementSpeed;
            cameraAcceleration = cameraAcceleration + -1.5f*gameState->cameraDPos;
            V3 cameraPosDelta = (0.5f*cameraAcceleration*square(deltaTimeSec) + gameState->cameraDPos*deltaTimeSec);
            gameState->cameraDPos = cameraAcceleration*deltaTimeSec + gameState->cameraDPos;
            V3 newCameraPosWorld = gameState->cameraPosWorld + cameraPosDelta;

            gameState->cameraCurrDistance += newCameraPosWorld.y - gameState->cameraPosWorld.y;
            if (gameState->cameraCurrDistance > gameState->cameraMaxDistance)
            {
                gameState->cameraCurrDistance = gameState->cameraMaxDistance;
            }
            else if (gameState->cameraCurrDistance < gameState->cameraMinDistance)
            {
                gameState->cameraCurrDistance = gameState->cameraMinDistance;
            }
        }

        r32 offsetFromCamera = (gameState->cameraCurrDistance + 0.001f) / sine(RADIANS(gameState->cameraIsometricHorizontalAxisRotationAngleDegrees));
        gameState->cameraPosWorld = cameraFollowingEntity->posWorld - offsetFromCamera * gameState->isometricN;

        gameState->mousePos[0] = gcInput->mouseX;
        gameState->mousePos[1] = gcInput->mouseY;

        gameState->n = gameState->isometricN;
        gameState->u = gameState->isometricU;
        gameState->v = gameState->isometricV;
    }


    M44 translationComponent = {1.0f, 0.0f, 0.0f, 0.0f,
                                0.0f, 1.0f, 0.0f, 0.0f,
                                0.0f, 0.0f, 1.0f, 0.0f,
                                -gameState->cameraPosWorld.x,
                                -gameState->cameraPosWorld.y,
                                -gameState->cameraPosWorld.z,
                                1.0f};
    M44 rotationComponent = {gameState->u.x, gameState->v.x, gameState->n.x, 0.0f,
                             gameState->u.y, gameState->v.y, gameState->n.y, 0.0f,
                             gameState->u.z, gameState->v.z, gameState->n.z, 0.0f,
                             0.0f,
                             0.0f,
                             0.0f,
                             1.0f};

    gameState->viewMatrix = rotationComponent * translationComponent;

    // NOTE(dima): creating projection matrix
    {
        r32 tanHalfFov = tangent(RADIANS(gameState->fov / 2.0f));
        r32 d = 1 / tanHalfFov;

        r32 aspectRatio = drawableWidthWithoutScaleFactor / drawableHeightWithoutScaleFactor;

        r32 a = gameState->farPlane / (gameState->farPlane - gameState->nearPlane);
        r32 b = (gameState->nearPlane*gameState->farPlane) / (gameState->nearPlane - gameState->farPlane);

        gameState->projectionMatrix = {d / aspectRatio, 0.0f, 0.0f, 0.0f,
                                       0.0f, d, 0.0f, 0.0f,
                                       0.0f, 0.0f, a, 1.0f,
                                       0.0f, 0.0f, b, 0.0f};
    }

    if (gcInput->mouseButtons[1].endedDown)
    {
        CastRayToClickPositionOnTilemap(gameState,
                                        drawableWidthWithoutScaleFactor,
                                        drawableHeightWithoutScaleFactor);
    }

#if 0
    V4 screenCenterPointPosCamera = v4(0.0f, 0.0f, gameState->near + 0.001f, 1.0f);
    V4 screenCenterPosNearPlaneWorld = inverseViewMatrix * screenCenterPointPosCamera;
#endif

    if (gameState->isClickPositionInsideTilemap)
    {
        pushRenderEntryLine(&renderGroupArena,
                            &renderGroup,
                            v3(0.0f, 1.0f, 0.0f),
                            gameState->cameraDirectonPosVectorStartWorld,
                            gameState->cameraDirectonPosVectorEndWorld);
    }

#if ANTIQUA_INTERNAL
    pushRenderEntryLine(&renderGroupArena,
                        &renderGroup,
                        v3(1.0f, 0.0f, 0.0f),
                        gameState->entities[gameState->cameraFollowingEntityIndex].posWorld,
                        gameState->entities[gameState->cameraFollowingEntityIndex].posWorld
                        + gameState->entities[gameState->cameraFollowingEntityIndex].dPos);
    pushRenderEntryLine(&renderGroupArena,
                        &renderGroup,
                        v3(1.0f, 0.0f, 0.0f),
                        gameState->entities[2].posWorld,
                        gameState->entities[2].posWorld
                        + gameState->entities[2].dPos);
    V3 cameraFollowingEntityCollisionPoint = gameState->collisionPointDebug;
    if (cameraFollowingEntityCollisionPoint.x != 0.0f
        || cameraFollowingEntityCollisionPoint.z != 0.0f)
    {
        pushRenderEntryLine(&renderGroupArena,
                            &renderGroup,
                            v3(0.0f, 1.0f, 0.0f),
                            cameraFollowingEntityCollisionPoint,
                            cameraFollowingEntityCollisionPoint
                            + gameState->lineNormalVector);
        pushRenderEntryPoint(&renderGroupArena,
                             &renderGroup,
                             cameraFollowingEntityCollisionPoint,
                             v3(1.0f, 1.0f, 0.0f));
    }

#if TEXTURE_DEBUG_MODE
    pushRenderEntryTextureDebug(&renderGroupArena,
                                &renderGroup,
                                gameState->font.atlasHeader,
                                gameState->font.needsGpuReupload);
    gameState->font.needsGpuReupload = false;
#else
    s8 text[] = "HeljoWorld!";
    pushRenderEntryText(&renderGroupArena,
                        &renderGroup,
                        text,
                        gameState->font.atlasHeader,
                        gameState->font.glyphMetadata,
                        gameState->font.firstGlyphCode,
                        false);

#endif

#if 0
    pushRenderEntryPoint(&renderGroupArena,
                         &renderGroup,
                         v3(screenCenterPosNearPlaneWorld),
                         v3(0.0f, 1.0f, 0.0f));
#endif
#endif

    renderGroup.uniforms[0] = gameState->viewMatrix;
    renderGroup.uniforms[1] = gameState->projectionMatrix;

    memory->renderOnGPU(0, &renderGroupArena, &renderGroup, (s32)drawableWidthWithoutScaleFactor, (s32)drawableHeightWithoutScaleFactor);

    // TODO(dima): on macos make everything run on the same thread. Get rid of these locks!!!
#if COMPILER_LLVM
    memory->waitIfInputBlocked(thread);
    memory->lockInputThread(thread);
    memory->resetInputStateButtons(thread);
    memory->unlockInputThread(thread);
#endif
}

#if !XCODE_BUILD && !COMPILER_MSVC
EXPORT MONExternC FILL_SOUND_BUFFER(fillSoundBuffer)
#else
FILL_SOUND_BUFFER(fillSoundBuffer)
#endif
{
#if 0
  // we're just filling the entire buffer here
  // In a real game we might only fill part of the buffer and set the mAudioDataBytes
  // accordingly.
  u32 framesToGen = soundState->needFrames;

  // calc the samples per up/down portion of each square wave (with 50% period)
  u32 framesPerCycle = soundState->sampleRate / soundState->toneHz;

  r32 *bufferPos = soundState->frames;
  r32 frameOffset = soundState->frameOffset;

  while (framesToGen) {
    // calc rounded frames to generate and accumulate fractional error
    u32 frames;
    u32 needFrames = (u32) (round(framesPerCycle - frameOffset));
    frameOffset -= framesPerCycle - needFrames;

    // we may be at the end of the buffer, if so, place offset at location in wave and clip
    if (needFrames > framesToGen) {
      frameOffset += framesToGen;
      frames = framesToGen;
    }
    else {
      frames = needFrames;
    }
    framesToGen -= frames;
    
    // simply put the samples in
    for (u32 x = 0; x < frames; ++x) {
#if 0
      r32 sineValue = sinf(soundState->tSine);
#else
      r32 sineValue = 0;
#endif
      r32 sample = sineValue;
      *bufferPos++ = sample;
      *bufferPos++ = sample;
      soundState->tSine += 2.f * PI32 * 1.f / framesPerCycle;
      if (soundState->tSine > 2.f * PI32)
      {
        soundState->tSine -= 2.f * PI32;
      }
    }
  }

  soundState->frameOffset = frameOffset;
#endif
}

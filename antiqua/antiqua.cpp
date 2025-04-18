#include "antiqua.h"
#include "stdio.h"
#include "antiqua_intrinsics.h"
#include "antiqua_render_group.h"

#include "antiqua_render_group.cpp"

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

            Rect entityRegularAABB = gameState->meshModels[entity->meshModelIndex].regularAxisAlignedBoundingBox;
            entityRegularAABB.diameterW *= entity->scaleFactor.x;
            entityRegularAABB.diameterH *= entity->scaleFactor.z;

            Rect testEntityRegularAABB = gameState->meshModels[testEntity->meshModelIndex].regularAxisAlignedBoundingBox;
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

#if !XCODE_BUILD && !COMPILER_MSVC
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
    /* TODO(dima):
       1. DONE add new struct type: MDLMesh. It should include array of submeshes that the model consists of
       2. DONE switch mesh loading code to use MDLMesh instead of RenderEntryMesh
       3. DONE Add logic to a) move center of mesh to 0;0;0 and b) lift mesh up so that all Y-coords are >= 0
       4. DONE Fix remaining compiler errors

       5. DONE Fix collision detection
    */

    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);
    GameState *gameState = (GameState *) memory->permanentStorage;

    MemoryArena mdlArena;
    u64 mdlArenaSize = MB(256);
    ASSERT(mdlArenaSize <= memory->permanentStorageSize - sizeof(gameState));
    initializeArena(&mdlArena,
                    mdlArenaSize,
                    (u8 *) memory->permanentStorage + sizeof(GameState));

    MemoryArena renderGroupArena;
    u32 renderGroupArenaSize = KB(256);
    ASSERT(renderGroupArenaSize <= memory->transientStorageSize);
    initializeArena(&renderGroupArena,
                    renderGroupArenaSize,
                    (u8 *) memory->transientStorage);

    MemoryArena scratchArena;
    u32 scratchArenaSize = KB(256);
    ASSERT(scratchArenaSize <= memory->transientStorageSize - renderGroupArenaSize);
    initializeArena(&scratchArena,
                    scratchArenaSize,
                    (u8 *) memory->transientStorage + renderGroupArenaSize);

    ASSERT(&gcInput->terminator - &gcInput->buttons[0] == ARRAY_COUNT(gcInput->buttons));
    if (!memory->isInitialized)
    {
        // do initialization here as needed

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
            newEntity->meshModelIndex = 0;
            gameState->entityCount++;
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Static);
            newEntity->posWorld = v3(5.0f, 0.0f, 5.0f);
            newEntity->scaleFactor = v3(20.0f, 1.0f, 1.0f);
            newEntity->meshModelIndex = 0;
            gameState->entityCount++;
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Moving | EntityFlags_Enemy);
            newEntity->posWorld = v3(6.5f, 0.0f, 7.5f);
            newEntity->scaleFactor = v3(1.0f, 2.0f, 1.0f);
            newEntity->meshModelIndex = 0;
            gameState->entityCount++;
        }

        gameState->cameraFollowingEntityIndex = 0;

        MeshMdl *meshModel = (MeshMdl *) (gameState->meshModels + gameState->meshModelCount);
        gameState->meshModelCount++;

        debug_ReadFileResult mdlFile = {};
#define FILENAME "data" DIR_SEPARATOR "test_cube.mdl"
        memory->debug_platformReadEntireFile(thread, &mdlFile, FILENAME);
#undef FILENAME
        ASSERT(mdlFile.contentsSize);
#define MAX_LINE_LENGTH 256
        s8* line = PUSH_ARRAY(&scratchArena, MAX_LINE_LENGTH, s8);
        s8 *tmp = PUSH_ARRAY(&scratchArena, MAX_LINE_LENGTH, s8);
        s8 *l, *fl, *tmpPtr, *linePtr;
        debug_ReadFileResult binFile = {};
        b32 binFileRead = false,
            minCornerRead = false,
            maxCornerRead = false,
            meshCountRead = false;
        V3 minCorner, maxCorner;
        u32 meshIndex = 0;
        for (l = line, fl = (s8 *) mdlFile.contents;
             fl - ((s8 *) mdlFile.contents) <= mdlFile.contentsSize;
             fl++)
        {
            ASSERT(l - line <= MAX_LINE_LENGTH);

            if (*fl == '\n')
            {
                if (*line == '#')
                {
                    goto continue_loop;
                }

                if (!binFileRead)
                {
                    memory->debug_platformReadEntireFile(thread, &binFile, "data/test_cube.bin");
                    ASSERT(binFile.contentsSize);

                    meshModel->data = (u8 *)binFile.contents;
                    meshModel->dataSize = binFile.contentsSize;

                    binFileRead = true;
                }
                else if (!meshCountRead)
                {
                    linePtr = line;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    u32 meshCount = asciiToU32OverflowUnsafe(tmp, tmpPtr - tmp);
                    meshModel->meshCount = meshCount;
                    meshModel->meshMtd = PUSH_ARRAY(&mdlArena, meshCount, MeshMetadata);

                    meshCountRead = true;
                }
                else if (!minCornerRead)
                {
                    linePtr = line;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    *tmpPtr = '\0';
                    minCorner.x = asciiToR32(tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    *tmpPtr = '\0';
                    minCorner.y = asciiToR32(tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    *tmpPtr = '\0';
                    minCorner.z = asciiToR32(tmp);

                    minCornerRead = true;
                }
                else if (!maxCornerRead)
                {
                    linePtr = line;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    *tmpPtr = '\0';
                    maxCorner.x = asciiToR32(tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    *tmpPtr = '\0';
                    maxCorner.y = asciiToR32(tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    *tmpPtr = '\0';
                    maxCorner.z = asciiToR32(tmp);

                    maxCornerRead = true;

                    meshModel->meshCenterAndMinY[0] = (minCorner.x + maxCorner.x) / 2;
                    meshModel->meshCenterAndMinY[1] = (minCorner.y + maxCorner.y) / 2;
                    meshModel->meshCenterAndMinY[2] = (minCorner.z + maxCorner.z) / 2;
                    meshModel->meshCenterAndMinY[3] = minCorner.y;

                    meshModel->regularAxisAlignedBoundingBox.diameterW = maxCorner.x - minCorner.x;
                    meshModel->regularAxisAlignedBoundingBox.diameterH = maxCorner.z - minCorner.z;
                }
                else
                {
                    MeshMetadata *meshMtd = meshModel->meshMtd + meshIndex;

                    linePtr = line;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    meshMtd->indicesByteOffset = asciiToU32OverflowUnsafe(tmp, tmpPtr - tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    meshMtd->indicesByteLength = asciiToU32OverflowUnsafe(tmp, tmpPtr - tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    meshMtd->indicesCount = asciiToU32OverflowUnsafe(tmp, tmpPtr - tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    meshMtd->posByteOffset = asciiToU32OverflowUnsafe(tmp, tmpPtr - tmp);

                    linePtr++;
                    tmpPtr = tmp;
                    while (*linePtr != ' ')
                    {
                        *tmpPtr++ = *linePtr++;
                    }
                    meshMtd->posByteLength = asciiToU32OverflowUnsafe(tmp, tmpPtr - tmp);

                    meshIndex++;
                }
                
continue_loop:
                l = line;
            }
            else
            {
                *l++ = *fl;
            }
        }

        memory->isInitialized = true;
    }

    if (gcInput->debugCmdC.endedDown)
    {
        gameState->debugCameraEnabled = !gameState->debugCameraEnabled;
    }

    RenderGroup renderGroup = {0};
    renderGroup.maxPushBufferSize = renderGroupArenaSize;
    renderGroup.pushBufferBase = renderGroupArena.base;

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

            ASSERT(entity->meshModelIndex < gameState->meshModelCount);
            MeshMdl *entityMeshMdl = gameState->meshModels + entity->meshModelIndex;

#if 1
            pushRenderEntryMesh(&renderGroupArena,
                                &renderGroup,
                                entity->posWorld,
                                entity->scaleFactor,
                                entityMeshMdl);
#endif
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

            ASSERT(entity->meshModelIndex < gameState->meshModelCount);
            MeshMdl *entityMeshMdl = gameState->meshModels + entity->meshModelIndex;

#if 1
            pushRenderEntryMesh(&renderGroupArena,
                                &renderGroup,
                                entity->posWorld,
                                entity->scaleFactor,
                                entityMeshMdl);
#endif
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

#if 0
    pushRenderEntryPoint(&renderGroupArena,
                         &renderGroup,
                         v3(screenCenterPosNearPlaneWorld),
                         v3(0.0f, 1.0f, 0.0f));
#endif
#endif

    renderGroup.uniforms[0] = gameState->viewMatrix;
    renderGroup.uniforms[1] = gameState->projectionMatrix;

    memory->renderOnGPU(0, &renderGroup, (s32)drawableWidthWithoutScaleFactor, (s32)drawableHeightWithoutScaleFactor);

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

#include "antiqua.h"
#include "types.h"
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
    mousePosNearPlaneCamera.z = gameState->near + 0.001f;

    V4 cameraDirectonPosVectorStartCamera = mousePosNearPlaneCamera;
    cameraDirectonPosVectorStartCamera.z = gameState->near + 0.001;
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

static void CalculateAABB(Entity *entity,
                          r32 *vertices,
                          u32 verticesSize)
{
    r32 minX = 0.0f, maxX = 0.0f, minZ = 0.0f, maxZ = 0.0f;
    for (u32 vertexIndex = 0;
         vertexIndex < verticesSize;
         vertexIndex += 3)
    {
        r32 x = vertices[vertexIndex];
        r32 z = vertices[vertexIndex + 2];

        if (x < minX)
        {
            minX = x;
        }
        if (x > maxX)
        {
            maxX = x;
        }

        if (z < minZ)
        {
            minZ = z;
        }
        if (z > maxZ)
        {
            maxZ = z;
        }
    }

    minX *= entity->scaleFactor.x;
    maxX *= entity->scaleFactor.x;

    minZ *= entity->scaleFactor.z;
    maxZ *= entity->scaleFactor.z;

    entity->regularAxisAlignedBoundingBox.diameterW = maxX - minX;
    entity->regularAxisAlignedBoundingBox.diameterH = maxZ - minZ;
}

static void MoveEntity(GameState *gameState,
                       Entity *testEntity,
                       u32 testEntityIndex,
                       r32 deltaTimeSec,
                       V3 acceleration,
                       V3 posDelta)
{
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

            Rect aabbAfterMinkowskiSum;
            aabbAfterMinkowskiSum.diameterW =
                (testEntity->regularAxisAlignedBoundingBox.diameterW
                 + entity->regularAxisAlignedBoundingBox.diameterW);
            aabbAfterMinkowskiSum.diameterH =
                (testEntity->regularAxisAlignedBoundingBox.diameterH
                 + entity->regularAxisAlignedBoundingBox.diameterH);

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
                           indices 2,3 - left bottom to left top
                           indices 4,5 - right bottom to right top
                           indices 6,7 - left top to right top */
            V3 lines[4*2];
            lines[0] = bottomLeft;
            lines[1] = bottomRight;
            lines[2] = bottomLeft;
            lines[3] = topLeft;
            lines[4] = bottomRight;
            lines[5] = topRight;
            lines[6] = topLeft;
            lines[7] = topRight;

            V3 ro = posWorld - entity->posWorld;
            ro.y = 0.0f;

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

                    b32 intersectAtEndpoints = ((numeratorT == 0
                                                || numeratorT == denominator
                                                || (numeratorU == 0
                                                    || numeratorU == denominator)));
                    b32 intersectNotAtEndpoints = (t > 0.0f
                                                   && t < 1.0f
                                                   && u > 0.0f
                                                   && u < 1.0f);

                    b32 testEntityMoving = testEntity->flags & EntityFlags_Moving;
                    b32 entityMoving = entity->flags & EntityFlags_Moving;
                    /* NOTE(dima): from purely mathematical standpoint, we would check only
                       non-endpoints to determine collisions - because our approach is
                       to never allow collisions in the first place. However, probably
                       due to floating point rounding errors, when both objects are moving,
                       sometimes they collide at endpoints. In this case, to avoid increasing
                       epsilon value, we are checking for intersection using
                       both endpoints and non-endpoints. We want to keep epsilon as low as
                       possible, because that ensures smoother collision response - less
                       backward jerking */
                    b32 collide = ((testEntityMoving && entityMoving)
                                   ? (intersectAtEndpoints || intersectNotAtEndpoints)
                                   : intersectNotAtEndpoints);

                    if (collide)
                    {
                        if (t < tMin)
                        {
                            tMin = t;

                            r32 dx = q1.x - qo.x;
                            r32 dz = q1.z - qo.z;
                            lineNormalVector.x = -dz;
                            lineNormalVector.z = dx;

                            V3 collisionPointPositionWorld = (posWorld
                                                              + tMin * posDelta);
                            gameState->collisionPointDebug = v3(collisionPointPositionWorld.x,
                                                             0.0f,
                                                             collisionPointPositionWorld.z);
                        }
                    }
                }
            }

            normalize(&lineNormalVector);
        }

        if (lineNormalVector.x != 0.0f
            || lineNormalVector.z != 0.0f)
        {
            gameState->lineNormalVector = lineNormalVector;
        }

        r32 posDeltaEpsilon = 0.005f;
        posWorld = posWorld + (posDelta*maximum(-posDeltaEpsilon, (tMin - posDeltaEpsilon)));
        posDelta = posDelta - (1*dot(posDelta, lineNormalVector) * lineNormalVector);
        testEntity->dPos = (testEntity->dPos
                            - (1*dot(testEntity->dPos, lineNormalVector)*lineNormalVector));
        tRemaining -= tRemaining*tMin;

        testEntity->posWorld.x = posWorld.x;
        testEntity->posWorld.z = posWorld.z;
    }
}

static void UpdatePlayer(GameState *gameState,
                         r32* vertices,
                         u32 verticesSize,
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
                        r32* vertices,
                        u32 verticesSize,
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
                               r32* vertices,
                               u32 verticesSize,
                               Entity *entity)
{
}

#if !XCODE_BUILD
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
    /* TODO(dima):
       - implement collision detection using AABB (DONE)
       - implement indexed drawing for meshes (DONE)
       - implement mesh importing from custom format based on glTF 2.0
    */

    s16 indices[] =
    {
        0,1,2,
        2,3,0,
        4,5,6,
        6,7,4,
        4,5,1,
        1,0,4,
        3,2,6,
        6,7,3,
        4,0,3,
        3,7,4,
        5,1,2,
        2,6,5
    };

    r32 vertices[] =
    {
        -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
        -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
        0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
        0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
        0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
        0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
    };

    {
        // NOTE(dima): move mesh center to (0;0;0)
        // TODO(dima): don't do it every frame - just do it once when loading a mesh
        r32 minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f, minZ = 0.0f, maxZ = 0.0f;
        for (u32 vertexIndex = 0;
             vertexIndex < ARRAY_COUNT(vertices);
             vertexIndex += 3)
        {
            r32 x = vertices[vertexIndex];
            r32 y = vertices[vertexIndex + 1];
            r32 z = vertices[vertexIndex + 2];

            if (x < minX)
            {
                minX = x;
            }
            if (x > maxX)
            {
                maxX = x;
            }

            if (y < minY)
            {
                minY = y;
            }
            if (y > maxY)
            {
                maxY = y;
            }

            if (z < minZ)
            {
                minZ = z;
            }
            if (z > maxZ)
            {
                maxZ = z;
            }
        }

        r32 centerX = (minX + maxX) / 2;
        r32 centerY = (minY + maxY) / 2;
        r32 centerZ = (minZ + maxZ) / 2;


        for (u32 vertexIndex = 0;
             vertexIndex < ARRAY_COUNT(vertices);
             vertexIndex += 3)
        {
            vertices[vertexIndex] -= centerX;
            vertices[vertexIndex + 1] -= centerY;
            vertices[vertexIndex + 2] -= centerZ;
        }
    }

    ASSERT(&gcInput->terminator - &gcInput->buttons[0] == ARRAY_COUNT(gcInput->buttons));
    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);

    GameState *gameState = (GameState *) memory->permanentStorage;
    if (!memory->isInitialized)
    {
        // do initialization here as needed

        gameState->near = 1.0f;
        gameState->far = 100.0f;
        gameState->fov = 45.0f;

        gameState->playerSpeed = 17.0f;

        gameState->cameraMinDistance = 5.0f;
        gameState->cameraMaxDistance = 20.0f;
        gameState->cameraCurrDistance = gameState->cameraMinDistance;

        gameState->cameraRotationSpeed = 0.3f;
        gameState->cameraMovementSpeed = 5.0f;

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
            newEntity->posWorld = v3(0.0f, 0.5f, 0.0f);
            newEntity->scaleFactor = v3(1.0f, 1.0f, 1.0f);
            gameState->entityCount++;

            CalculateAABB(newEntity, vertices, ARRAY_COUNT(vertices));
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Static);
            newEntity->posWorld = v3(5.0f, 0.5f, 5.0f);
            newEntity->scaleFactor = v3(20.0f, 1.0f, 1.0f);
            gameState->entityCount++;

            CalculateAABB(newEntity, vertices, ARRAY_COUNT(vertices));
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->flags = (EntityFlags)(EntityFlags_Moving | EntityFlags_Enemy);
            newEntity->posWorld = v3(7.0f, 1.0f, 6.0f);
            newEntity->scaleFactor = v3(1.0f, 2.0f, 1.0f);
            gameState->entityCount++;

            CalculateAABB(newEntity, vertices, ARRAY_COUNT(vertices));
        }

        gameState->cameraFollowingEntityIndex = 0;

        memory->isInitialized = true;
    }

    if (gcInput->scrollingDeltaY != 0.0f)
    {
        gameState->cameraCurrDistance += gameState->cameraMovementSpeed * deltaTimeSec * -gcInput->scrollingDeltaY;
        if (gameState->cameraCurrDistance > gameState->cameraMaxDistance)
        {
            gameState->cameraCurrDistance = gameState->cameraMaxDistance;
        }
        else if (gameState->cameraCurrDistance < gameState->cameraMinDistance)
        {
            gameState->cameraCurrDistance = gameState->cameraMinDistance;
        }
    }

    if (gcInput->debugCmdC.endedDown)
    {
        gameState->debugCameraEnabled = !gameState->debugCameraEnabled;
    }

    MemoryArena renderGroupArena;
    u64 renderGroupArenaSize = KB(256);
    ASSERT(renderGroupArenaSize <= memory->transientStorageSize);
    initializeArena(&renderGroupArena,
                    KB(256),
                    (u8 *) memory->transientStorage);

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

            pushRenderEntryMesh(&renderGroupArena,
                                &renderGroup,
                                entity->posWorld,
                                entity->scaleFactor,
                                vertices,
                                ARRAY_COUNT(vertices),
                                indices,
                                ARRAY_COUNT(indices));
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
                V3 diagonalDirectionVector = vWithoutY + uWithoutY;
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else if (gcInput->down.endedDown)
            {
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
                V3 diagonalDirectionVector = vWithoutY + uWithoutY;
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else if (gcInput->down.endedDown)
            {
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
                             vertices,
                             ARRAY_COUNT(vertices),
                             entity,
                             entityIndex,
                             deltaTimeSec,
                             playerAcceleration);
            }
            else if ((flags & EntityFlags_Enemy) == EntityFlags_Enemy)
            {
                UpdateEnemy(gameState,
                            vertices,
                            ARRAY_COUNT(vertices),
                            entity,
                            entityIndex,
                            deltaTimeSec);
            }
            else if ((flags & EntityFlags_Static) == EntityFlags_Static)
            {
                /* TODO(dima): this only calculates AABB!
                               DON'T do it every frame. Do it only once at loading! */
                UpdateStaticEntity(gameState,
                                   vertices,
                                   ARRAY_COUNT(vertices),
                                   entity);
            }
            else
            {
                INVALID_CODE_PATH;
            }

            pushRenderEntryMesh(&renderGroupArena,
                                &renderGroup,
                                entity->posWorld,
                                entity->scaleFactor,
                                vertices,
                                ARRAY_COUNT(vertices),
                                indices,
                                ARRAY_COUNT(indices));
        }

        // NOTE(dima): set up camera to look at entity it follows
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

        r32 a = gameState->far / (gameState->far - gameState->near);
        r32 b = (gameState->near * gameState->far) / (gameState->near - gameState->far);

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

    memory->renderOnGpu(0, &renderGroup);

    memory->waitIfInputBlocked(thread);
    memory->lockInputThread(thread);
    memory->resetInputStateButtons(thread);
    memory->unlockInputThread(thread);
}

#if !XCODE_BUILD
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

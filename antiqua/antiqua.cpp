#include "antiqua.h"
#include "types.h"
#include "stdio.h"
#include "antiqua_intrinsics.h"
#include "antiqua_render_group.h"

#include "antiqua_render_group.cpp"

#if !XCODE_BUILD
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
    /* TODO(dima):
       - implement collision detection using bounding circles and/or rectangles
     */

    r32 vertices[] =
    {
        // front face
        -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
        1.0f, 0.0f, 0.0f, // color = red
        -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
        1.0f, 0.0f, 0.0f, // color = red
        0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
        1.0f, 0.0f, 0.0f, // color = red
        0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
        1.0f, 0.0f, 0.0f, // color = red
        0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
        1.0f, 0.0f, 0.0f, // color = red
        -0.5f, 0.5f, 0.0f, // vertex #0 - coordinates
        1.0f, 0.0f, 0.0f, // color = red
        // back face
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        0.0f, 1.0f, 0.0f, // color = green
        -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
        0.0f, 1.0f, 0.0f, // color = green
        0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
        0.0f, 1.0f, 0.0f, // color = green
        0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
        0.0f, 1.0f, 0.0f, // color = green
        0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
        0.0f, 1.0f, 0.0f, // color = green
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        0.0f, 1.0f, 0.0f, // color = green
        // left face
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        0.0f, 0.0f, 1.0f, // color = blue
        -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
        0.0f, 0.0f, 1.0f, // color = blue
        -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
        0.0f, 0.0f, 1.0f, // color = blue
        -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
        0.0f, 0.0f, 1.0f, // color = blue
        -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
        0.0f, 0.0f, 1.0f, // color = blue
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        0.0f, 0.0f, 1.0f, // color = blue
        // right face
        0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
        1.0f, 0.4f, 0.0f, // color = orange
        0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
        1.0f, 0.4f, 0.0f, // color = orange
        0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
        1.0f, 0.4f, 0.0f, // color = orange
        0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
        1.0f, 0.4f, 0.0f, // color = orange
        0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
        1.0f, 0.4f, 0.0f, // color = orange
        0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
        1.0f, 0.4f, 0.0f, // color = orange
        // top face
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        1.0f, 0.0f, 1.0f, // color = purple
        -0.5f, 0.5f, 0.0f,  // vertex #0 - coordinates
        1.0f, 0.0f, 1.0f, // color = purple
        0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
        1.0f, 0.0f, 1.0f, // color = purple
        0.5f, 0.5f, 0.0f,   // vertex #3 - coordinates
        1.0f, 0.0f, 1.0f, // color = purple
        0.5f, 0.5f, 1.0f,    // vertex #7 - coordinates
        1.0f, 0.0f, 1.0f, // color = purple
        -0.5f, 0.5f, 1.0f,  // vertex #4 - coordinates
        1.0f, 0.0f, 1.0f, // color = purple
        // bottom face
        -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
        0.0f, 1.0f, 1.0f, // color = cyan
        -0.5f, -0.5f, 0.0f,  // vertex #1 - coordinates
        0.0f, 1.0f, 1.0f, // color = cyan
        0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
        0.0f, 1.0f, 1.0f, // color = cyan
        0.5f, -0.5f, 0.0f,   // vertex #2 - coordinates
        0.0f, 1.0f, 1.0f, // color = cyan
        0.5f, -0.5f, 1.0f,   // vertex #6 - coordinates
        0.0f, 1.0f, 1.0f, // color = cyan
        -0.5f, -0.5f, 1.0f,  // vertex #5 - coordinates
        0.0f, 1.0f, 1.0f, // color = cyan
    };

    {

        // NOTE(dima): move mesh center to (0;0;0)
        // TODO(dima): don't do it every frame - just do it once when loading a mesh
        r32 minX = 0.0f, maxX = 0.0f, minY = 0.0f, maxY = 0.0f, minZ = 0.0f, maxZ = 0.0f;
        for (u32 vertexIndex = 0;
             vertexIndex < ARRAY_COUNT(vertices) - 3;
             vertexIndex += 6)
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
             vertexIndex < ARRAY_COUNT(vertices) - 3;
             vertexIndex += 6)
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

        gameState->playerSpeed = 20.0f;

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
            newEntity->type = EntityType_Cube;
            newEntity->positionWorld = v3(0.0f, 0.5f, 0.0f);
            newEntity->scaleFactor = v3(1.0f, 1.0f, 1.0f);
            gameState->entityCount++;
        }
        {
            Entity *newEntity = (Entity *)(gameState->entities + gameState->entityCount);
            newEntity->type = EntityType_Cube;
            newEntity->positionWorld = v3(5.0f, 0.5f, 5.0f);
#if 1
            newEntity->scaleFactor = v3(20.0f, 1.0f, 1.0f);
#else
            newEntity->scaleFactor = v3(1.0f, 1.0f, 1.0f);
#endif
            gameState->entityCount++;
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

    V3 boundingEllipsePoints[4];

    V3 tangentVectorNormalized;
    V3 collisionPointDebug;
    r32 zAtZero;
    V3 tangentNormalVector = {0};
    V3 reflectedDPositionVector;
    V3 debugPlayerPosition;

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
            normalize(&uWithoutY);
            V3 vWithoutY = v3(gameState->v.x, 0.0f, gameState->v.z);
            r32 vWithoutYLength = length(vWithoutY);
            vWithoutY = (1.0f / vWithoutYLength) * vWithoutY;

            if (gcInput->up.endedDown)
            {
                V3 diagonalDirectionVector = vWithoutY + uWithoutY;
                normalize(&diagonalDirectionVector);
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else if (gcInput->down.endedDown)
            {
                V3 diagonalDirectionVector = -vWithoutY + uWithoutY;
                normalize(&diagonalDirectionVector);
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
            normalize(&uWithoutY);
            V3 vWithoutY = v3(gameState->v.x, 0.0f, gameState->v.z);
            r32 vWithoutYLength = length(vWithoutY);
            vWithoutY = (1.0f / vWithoutYLength) * vWithoutY;

            if (gcInput->up.endedDown)
            {
                V3 diagonalDirectionVector = vWithoutY + uWithoutY;
                normalize(&diagonalDirectionVector);
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else if (gcInput->down.endedDown)
            {
                V3 diagonalDirectionVector = -vWithoutY + uWithoutY;
                normalize(&diagonalDirectionVector);
                playerAcceleration.x = diagonalDirectionVector.x;
                playerAcceleration.z = diagonalDirectionVector.z;
            }
            else
            {
                playerAcceleration.x = uWithoutY.x;
                playerAcceleration.z = uWithoutY.z;
            }
        }

        playerAcceleration *= gameState->playerSpeed;
        playerAcceleration = playerAcceleration + -3.0f * gameState->dPlayerPosition;

        V3 playerDelta = 0.5f * playerAcceleration * square(deltaTimeSec) + gameState->dPlayerPosition * deltaTimeSec;
        gameState->dPlayerPosition = playerAcceleration * deltaTimeSec + gameState->dPlayerPosition;

        // NOTE(dima): collision detection and response using bounding ellipse
        r32 epsilon = 0.00001f;
        V3 playerPositionWorld = {0};

        r32 tRemaining = 1.0f;
        for (u32 iteration = 0;
             iteration < 4 && tRemaining > 0.0f;
             ++iteration)
        {
            r32 tMin = 1.0f;

            Entity *entity = 0;
            for (u32 entityIndex = 0;
                 entityIndex < gameState->entityCount;
                 ++entityIndex)
            {
                entity = gameState->entities + entityIndex;

                r32 minX = 0.0f, maxX = 0.0f, minZ = 0.0f, maxZ = 0.0f;
                for (u32 vertexIndex = 0;
                     vertexIndex < ARRAY_COUNT(vertices) - 3;
                     vertexIndex += 6)
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

                entity->axisAlignedBoundingEllipseRadiuses.x = absoluteValue((maxX - minX) / 2);
                entity->axisAlignedBoundingEllipseRadiuses.y = absoluteValue((maxZ - minZ) / 2);

                Entity *cameraFollowingEntity = gameState->entities + gameState->cameraFollowingEntityIndex;

                if (entityIndex != gameState->cameraFollowingEntityIndex)
                {
                    playerPositionWorld = v3(cameraFollowingEntity->positionWorld.x,
                                                   0.0f,
                                                   cameraFollowingEntity->positionWorld.z);
                    V3 ro = playerPositionWorld - entity->positionWorld;
                    ro.y = 0.0f;
                    r32 rax = (entity->axisAlignedBoundingEllipseRadiuses.x + cameraFollowingEntity->axisAlignedBoundingEllipseRadiuses.x) * squareRoot(2.0f);
                    r32 raz = (entity->axisAlignedBoundingEllipseRadiuses.y + cameraFollowingEntity->axisAlignedBoundingEllipseRadiuses.y) * squareRoot(2.0f);
                    r32 rdx = gameState->dPlayerPosition.x;
                    r32 rdz = gameState->dPlayerPosition.z;
                    r32 rox = ro.x;
                    r32 roz = ro.z;

                    // NOTE(dima): for visualizing ellipse only
                    // left
                    boundingEllipsePoints[0] = v3(entity->positionWorld.x - rax, 0.0f, entity->positionWorld.z);
                    // right
                    boundingEllipsePoints[1] = v3(entity->positionWorld.x + rax, 0.0f, entity->positionWorld.z);
                    // top
                    boundingEllipsePoints[2] = v3(entity->positionWorld.x, 0.0f, entity->positionWorld.z + raz);
                    // bottom
                    boundingEllipsePoints[3] = v3(entity->positionWorld.x, 0.0f, entity->positionWorld.z - raz);

                    r32 a = raz*raz*rdx*rdx + rax*rax*rdz*rdz;
                    r32 b = 2*raz*raz*rox*rdx + 2*rax*rax*roz*rdz;
                    r32 c = raz*raz*rox*rox + rax*rax*roz*roz - rax*rax*raz*raz;

                    r32 d = b*b - 4*a*c;

                    if (d >= 0)
                    {
                        r32 tOne = (-b + squareRoot(d)) / (2*a);
                        r32 tTwo = (-b - squareRoot(d)) / (2*a);
                        if (tOne >= 0 || tTwo >= 0)
                        {
                            r32 distanceTillCollision = minimum(tOne, tTwo) - epsilon;
                            distanceTillCollision = length(gameState->dPlayerPosition*distanceTillCollision);

                            r32 maxPossibleDistanceForCurrentFrame = length(playerDelta);
                            if (maxPossibleDistanceForCurrentFrame >= 0 &&
                                maxPossibleDistanceForCurrentFrame >= distanceTillCollision)
                            {
                                tMin = distanceTillCollision / maxPossibleDistanceForCurrentFrame;
                                if (tMin < 0.0f)
                                {
                                    tMin = 0.0f;
                                }

                                V3 collisionPointPositionWorld = playerPositionWorld + tMin * playerDelta;
                                collisionPointPositionWorld.y = 0.0f;
                                collisionPointDebug = v3(collisionPointPositionWorld.x, collisionPointPositionWorld.y, collisionPointPositionWorld.z);

                                r32 tangentSlope = -((collisionPointPositionWorld.x - entity->positionWorld.x) * square(raz)) / ((collisionPointPositionWorld.z - entity->positionWorld.z) * square(rax));
                                zAtZero = collisionPointPositionWorld.z - tangentSlope * collisionPointPositionWorld.x;
                                V3 tangentVector = v3(collisionPointPositionWorld.x - 0.0f, 0.0f, collisionPointPositionWorld.z - zAtZero);
                                r32 tangentVectorLength = length(tangentVector);
                                tangentVectorNormalized = (1.0f / tangentVectorLength) * tangentVector;

                                tangentNormalVector = v3(tangentSlope, 0.0f, -1.0f);
                                r32 tangentNormalVectorLength = length(tangentNormalVector);
                                tangentNormalVector = (1.0f / tangentNormalVectorLength) * tangentNormalVector;
                                reflectedDPositionVector = gameState->dPlayerPosition - 1*dot(gameState->dPlayerPosition, tangentNormalVector) * tangentNormalVector;
                            }
                        }
                    }
                }
            }

            playerDelta = playerDelta - (1*dot(playerDelta, tangentNormalVector) * tangentNormalVector);

            playerPositionWorld = playerPositionWorld + (playerDelta*tMin);

            debugPlayerPosition = playerPositionWorld;

            gameState->dPlayerPosition = gameState->dPlayerPosition - (1*dot(gameState->dPlayerPosition, tangentNormalVector) * tangentNormalVector);

            tRemaining -= tRemaining*tMin;

            cameraFollowingEntity->positionWorld.x = playerPositionWorld.x;
            cameraFollowingEntity->positionWorld.z = playerPositionWorld.z;
        }

        // NOTE(dima): set up camera to look at entity it follows
        r32 offsetFromCamera = (gameState->cameraCurrDistance + 0.001f) / sine(RADIANS(gameState->cameraIsometricHorizontalAxisRotationAngleDegrees));
        gameState->cameraPosWorld = cameraFollowingEntity->positionWorld - offsetFromCamera * gameState->isometricN;

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
    V4 mousePosNearPlaneWorld = inverseViewMatrix * mousePosNearPlaneCamera;

    if (gcInput->mouseButtons[1].endedDown)
    {
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

    V4 screenCenterPointPosCamera = v4(0.0f, 0.0f, gameState->near + 0.001f, 1.0f);
    V4 screenCenterPosNearPlaneWorld = inverseViewMatrix * screenCenterPointPosCamera;

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

    for (s32 entityIndex = gameState->entityCount - 1;
         entityIndex >= 0;
         --entityIndex)
    {
        Entity *entity = gameState->entities + entityIndex;
        pushRenderEntryMesh(&renderGroupArena,
                            &renderGroup,
                            entity->positionWorld,
                            entity->scaleFactor,
                            vertices,
                            ARRAY_COUNT(vertices));
    }

    pushRenderEntryPoint(&renderGroupArena,
                         &renderGroup,
                         v3(mousePosNearPlaneWorld),
                         v3(0.0f, 1.0f, 1.0f));
    if (gameState->isClickPositionInsideTilemap)
    {
        pushRenderEntryLine(&renderGroupArena,
                            &renderGroup,
                            v3(0.0f, 1.0f, 0.0f),
                            gameState->cameraDirectonPosVectorStartWorld,
                            gameState->cameraDirectonPosVectorEndWorld);
    }
    for (u32 ellipsePointIndex = 0;
         ellipsePointIndex < ARRAY_COUNT(boundingEllipsePoints);
         ++ellipsePointIndex)
    {
        pushRenderEntryPoint(&renderGroupArena,
                             &renderGroup,
                             boundingEllipsePoints[ellipsePointIndex],
                             v3(1.0f, 0.0f, 0.0f));
    }
    pushRenderEntryLine(&renderGroupArena,
                        &renderGroup,
                        v3(1.0f, 0.0f, 0.0f),
                        gameState->entities[0].positionWorld,
                        gameState->entities[0].positionWorld + gameState->dPlayerPosition);
    pushRenderEntryPoint(&renderGroupArena,
                         &renderGroup,
                         collisionPointDebug,
                         v3(1.0f, 1.0f, 0.0f));
    debugPlayerPosition.y = 0.0f;
    pushRenderEntryPoint(&renderGroupArena,
                         &renderGroup,
                         v3(screenCenterPosNearPlaneWorld),
                         v3(0.0f, 1.0f, 0.0f));

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

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
       - render a point at the center of the screen
       - implement ray casting through the center of the screen; draw line to visualize ray
       - finish implementing movement of isometric camera with WASD
     */
    ASSERT(&gcInput->terminator - &gcInput->buttons[0] == ARRAY_COUNT(gcInput->buttons));
    ASSERT(sizeof(GameState) <= memory->permanentStorageSize);

    GameState *gameState = (GameState *) memory->permanentStorage;
    if (!memory->isInitialized)
    {
        // do initialization here as needed

        gameState->worldMatrix = {1.0f, 0.0f, 0.0f, 0.0f,
                                  0.0f, 1.0f, 0.0f, 0.0f,
                                  0.0f, 0.0f, 1.0f, 0.0f,
                                  0.0f, 0.0f, 0.0f, 1.0f};

        gameState->cameraRotationSpeed = 0.3f;
        gameState->cameraMovementSpeed = 0.1f;

        gameState->cameraPosWorld = v3(-2.5f, 2.0f, -1.5f);
        gameState->v = v3(0.0f, 1.0f, 0.0f);
        gameState->n = v3(0.0f, 0.0f, 1.0f);
        gameState->u = cross(gameState->n, gameState->v);

#if 0
        // NOTE(dima): Below is isometric camera setup
        // NOTE(dima): rotate around vertical axis by 45 degrees
        rotate(&gameState->n, gameState->v, 45.0f);

        // NOTE(dima): rotate around horizontal axis by 45 degrees
        gameState->u = cross(gameState->v, gameState->n);
        normalize(&gameState->u);
        rotate(&gameState->n, gameState->u, 45.0f);

        gameState->v = cross(gameState->n, gameState->u);
        normalize(&gameState->v);
#endif

        memory->isInitialized = true;
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
    pushRenderEntryLine(&renderGroupArena,
                        &renderGroup,
                        v3(1.0f, 0.0f, 0.0f),
                        v3(0.75f, 0.75f, 0.5f),
                        v3(1.25f, 1.25f, 0.5f));
    pushRenderEntryMesh(&renderGroupArena,
                        &renderGroup,
                        vertices,
                        ARRAY_COUNT(vertices));

    if (gcInput->right.endedDown)
    {
        V3 u = cross(gameState->v, gameState->n);
        normalize(&u);
        gameState->cameraPosWorld = gameState->cameraPosWorld
                                    + gameState->cameraMovementSpeed * u;
    }

    if (gcInput->left.endedDown)
    {
        V3 u = cross(gameState->n, gameState->v);
        normalize(&u);
        gameState->cameraPosWorld = gameState->cameraPosWorld
                                    + gameState->cameraMovementSpeed * u;
    }

#if 1
    if (gcInput->up.endedDown)
    {
        gameState->cameraPosWorld = gameState->cameraPosWorld
                                    + gameState->cameraMovementSpeed * gameState->n;
    }

    if (gcInput->down.endedDown)
    {
        gameState->cameraPosWorld = gameState->cameraPosWorld
                                    - gameState->cameraMovementSpeed * gameState->n;
    }
#else
    if (gcInput->up.endedDown)
    {
        gameState->cameraPosWorld = gameState->cameraPosWorld
                                    + gameState->cameraMovementSpeed * v3(gameState->v.x, 0.0f, gameState->v.z);
    }

    if (gcInput->down.endedDown)
    {
        gameState->cameraPosWorld = gameState->cameraPosWorld
                                    - gameState->cameraMovementSpeed * v3(gameState->v.x, 0.0f, gameState->v.z);
    }
#endif

//    fprintf(stderr, "mouse x, y: %d, %d\n", gcInput->mouseX, gcInput->mouseY);
    static s32 mousePos[2] = {500, 350};
    static r32 cameraVerticalAngle = 0.0f;
    static r32 cameraHorizontalAngle = 0.0f;
//    if (gcInput->mouseButtons[0].endedDown
    if (true
        && (gcInput->mouseX != mousePos[0] || gcInput->mouseY != mousePos[1]))
    {
        V3 v = v3(0.0f, 1.0f, 0.0f);
        V3 n = v3(0.0f, 0.0f, 1.0f);
        V3 u = cross(n, v);

        r32 horizontalOffset = gcInput->mouseX - mousePos[0];
        r32 verticalOffset = gcInput->mouseY - mousePos[1];

        cameraVerticalAngle += gameState->cameraRotationSpeed * verticalOffset; 
        cameraHorizontalAngle += gameState->cameraRotationSpeed * horizontalOffset;

        // NOTE(dima): rotate by horizontal angle around vertical axis
        rotate(&n, v, cameraHorizontalAngle);

        // NOTE(dima): rotate by vertical angle around horizontal axis
        u = cross(v, n);
        normalize(&u);
        rotate(&n, u, cameraVerticalAngle);

        v = cross(n, u);
        normalize(&v);

        gameState->n = n;
        gameState->v = v;
        gameState->u = u;

        mousePos[0] = gcInput->mouseX;
        mousePos[1] = gcInput->mouseY;
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

#if 0
    // NOTE(dima): WIP ray tracing
    r32 mouseX = mousePos[0];
    r32 mouseY = mousePos[1];

    r32 xScale = 2.0f / 1000;
    r32 clipX = mouseX * xScale - 1.0f;
    r32 yScale = 2.0f / 700;
    r32 clipY = mouseY * yScale - 1.0f;

    V4 clipPos = v4(clipX, clipY, near, 1.0f);
    M44 inverseProjectionMatrix = inverse(&projectionMatrix);
    M44 inverseViewMatrix = inverse(&viewMatrix);
    V4 mousePosNearPlaneWorld = inverseViewMatrix * inverseProjectionMatrix * clipPos;
    fprintf(stderr, "mousePosNearPlaneWorld - x y: %f %f\n", mousePosNearPlaneWorld.x, mousePosNearPlaneWorld.y);
#endif

    renderGroup.uniforms[0] = gameState->worldMatrix;
    renderGroup.uniforms[1] = gameState->viewMatrix;

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

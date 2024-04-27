#include "antiqua.h"
#include "types.h"
#include "stdio.h"
#include "antiqua_intrinsics.h"
#include "antiqua_render_group.h"

#if !XCODE_BUILD
EXPORT MONExternC UPDATE_GAME_AND_RENDER(updateGameAndRender)
#else
UPDATE_GAME_AND_RENDER(updateGameAndRender)
#endif
{
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

        r32 fov = 45.0f; //90.0f;
        r32 tanHalfFov = tangent(RADIANS(fov / 2.0f));
        r32 d = 1 / tanHalfFov;

        r32 aspectRatio = 1000.0f / 700.0f;

        r32 near = 1.0f;
        r32 far = 100.0f;
        r32 a = far / (far - near);
        r32 b = (near * far) / (near - far);

        gameState->projectionMatrix = {d / aspectRatio, 0.0f, 0.0f, 0.0f,
                                       0.0f, d, 0.0f, 0.0f,
                                       0.0f, 0.0f, a, 1.0f,
                                       0.0f, 0.0f, b, 0.0f};

        gameState->cameraRotationSpeed = 0.3f;
        gameState->cameraMovementSpeed = 0.1f;

        gameState->cameraPosWorld = v3(0.0f, 0.0f, -6.0f);
        gameState->v = v3(0.0f, 1.0f, 0.0f);
        gameState->n = v3(0.0f, 0.0f, 1.0f);
        gameState->u = cross(gameState->n, gameState->v);

#if 0
        // NOTE(dima): rotate around vertical axis by 45 degrees
        rotate(&gameState->n, gameState->v, 45.0f);

        // NOTE(dima): rotate around horizontal axis by 45 degrees
        u = cross(gameState->v, gameState->n);
        normalize(&u);
        rotate(&gameState->n->u, 45.0f);

        gameState->v = cross(gameState->n, u);
        normalize(&gameState->v);
#endif

        memory->isInitialized = true;
    }

    MemoryArena renderGroupArena;
    MemoryIndex renderGroupArenaSize = KB(256);
    ASSERT(renderGroupArenaSize <= memory->transientStorageSize);
    initializeArena(&renderGroupArena,
                    KB(256),
                    (u8 *) memory->transientStorage);

    RenderGroup renderGroup = {0};
    renderGroup.maxPushBufferSize = KB(128);
    renderGroup.pushBufferBase = PUSH_SIZE(&renderGroupArena, renderGroup.maxPushBufferSize, u8);

    RenderGroupEntryHeader *entryClearHeader = (RenderGroupEntryHeader *)renderGroup.pushBufferBase + renderGroup.pushBufferSize;
    entryClearHeader->type = RenderGroupEntryType_RenderEntryClear;
    renderGroup.pushBufferSize += sizeof(RenderGroupEntryHeader);
    RenderEntryClear *entryClear = (RenderEntryClear *)renderGroup.pushBufferBase + renderGroup.pushBufferSize;
    entryClear->color = { v4(0.0f, 1.0f, 0.0f, 1.0f) };
    renderGroup.pushBufferSize += sizeof(RenderEntryClear);
    entryClearHeader->next = (RenderGroupEntryHeader *)renderGroup.pushBufferBase + renderGroup.pushBufferSize;

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
//        V3 v = gameState->v;
//        V3 n = gameState->n;
        V3 u = cross(n, v);

        r32 horizontalOffset = gcInput->mouseX - mousePos[0];
        r32 verticalOffset = gcInput->mouseY - mousePos[1];

//        cameraVerticalAngle += verticalOffset / 10.0f;
//        cameraHorizontalAngle += horizontalOffset / 20.0f;
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

    gameState->viewMatrix = {gameState->u.x, gameState->v.x, gameState->n.x, 0.0f,
                             gameState->u.y, gameState->v.y, gameState->n.y, 0.0f,
                             gameState->u.z, gameState->v.z, gameState->n.z, 0.0f,
                             -gameState->cameraPosWorld.x,
                             -gameState->cameraPosWorld.y,
                             -gameState->cameraPosWorld.z,
                             1.0f};

//    r32 mouseX = mousePos[0];
//    r32 mouseY = mousePos[1];
//
//    r32 xScale = 2.0f / 1000;
//    r32 clipX = mouseX * xScale - 1.0f;
//    r32 yScale = 2.0f / 700;
//    r32 clipY = mouseY * yScale - 1.0f;
//
//    V4 clipPos = v4(clipX, clipY, near, 1.0f);
//    M44 inverseProjectionMatrix = inverse(&projectionMatrix);
//    M44 inverseViewMatrix = inverse(&viewMatrix);
//    V4 mousePosNearPlaneWorld = inverseViewMatrix * inverseProjectionMatrix * clipPos;
//    fprintf(stderr, "mousePosNearPlaneWorld - x y: %f %f\n", mousePosNearPlaneWorld.x, mousePosNearPlaneWorld.y);

    renderGroup.uniforms[0] = gameState->worldMatrix;
    renderGroup.uniforms[1] = gameState->viewMatrix;
    renderGroup.uniforms[2] = gameState->projectionMatrix;

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

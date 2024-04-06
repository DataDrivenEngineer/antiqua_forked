#include "antiqua.h"
#include "types.h"
#include "stdio.h"
#include "antiqua_intrinsics.h"

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
    }

    MemoryArena renderGroupArena;
    MemoryIndex renderGroupArenaSize = KB(256);
    ASSERT(renderGroupArenaSize <= memory->transientStorageSize);
    initializeArena(&renderGroupArena,
                    KB(256),
                    (u8 *) memory->transientStorage);

    r32 fov = 90.0f;
    r32 tanHalfFov = tangent(RADIANS(fov / 2.0f));
    r32 d = 1 / tanHalfFov;

    r32 aspectRatio = 1000.0f / 700.0f;

    r32 near = 1.0f;
    r32 far = 100.0f;
    r32 a = far / (far - near);
    r32 b = (near * far) / (near - far);

    M44 projectionMatrix = {d / aspectRatio, 0.0f, 0.0f, 0.0f,
                            0.0f, d, 0.0f, 0.0f,
                            0.0f, 0.0f, a, 1.0f,
                            0.0f, 0.0f, b, 0.0f};

    V3 cameraPos = v3(0.2f, 0.0f, -6.0f);
    V3 u = v3(1.0f, 0.0f, 0.0f);
    V3 v = v3(0.0f, 1.0f, 0.0f);
    V3 n = v3(0.0f, 0.0f, 1.0f);

    // NOTE(dima): rotate around vertical axis by 45 degrees
    rotate(&n, v, 45.0f);

    // NOTE(dima): rotate around horizontal axis by 45 degrees
    u = cross(v, n);
    normalize(&u);
    rotate(&n, u, 45.0f);

    v = cross(n, u);
    normalize(&v);

    M44 viewMatrix = {u.x, v.x, n.x, 0.0f,
                      u.y, v.y, n.y, 0.0f,
                      u.z, v.z, n.z, 0.0f,
                      -cameraPos.x, -cameraPos.y, -cameraPos.z, 1.0f};

    M44 uniforms[2] = { viewMatrix, projectionMatrix };

    memory->renderOnGpu((r32 *)uniforms, sizeof(uniforms));

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

#ifndef osx_audio_h
#define osx_audio_h

#include <CoreAudio/AudioHardware.h>

#include "types.h"
#include "antiqua.h"

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

extern u8 soundPlaying;
extern struct SoundState soundState;

MONExternC u8 playAudio(void);
u8 stopAudio(void);
MONExternC void initAudio(void);
OSStatus resetAudio(void);

#endif

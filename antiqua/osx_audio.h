#ifndef osx_audio_h
#define osx_audio_h

#include <CoreAudio/AudioHardware.h>

#include "types.h"
#include "antiqua.h"

extern struct SoundState soundState;

void initAudio(void);
b32 playAudio(void);

b32 stopAudio(void);
OSStatus resetAudio(void);

#endif

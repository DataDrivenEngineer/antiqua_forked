#ifndef osx_audio_h
#define osx_audio_h

#include <CoreAudio/AudioHardware.h>

#include "types.h"

extern SoundState soundState;

void initAudio(void);
b32 playAudio(void);

b32 stopAudio(void);
OSStatus resetAudio(void);

#endif

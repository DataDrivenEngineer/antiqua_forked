#ifndef osx_audio_h
#define osx_audio_h

#include <CoreAudio/AudioHardware.h>

#include "types.h"
#include "antiqua.h"

extern u8 soundPlaying;

u8 playAudio(void);
u8 stopAudio(void);
void initAudio(void);
void resetAudio(void);

#endif

#ifndef osx_audio_h
#define osx_audio_h

#include <CoreAudio/AudioHardware.h>

#include "types.h"
#include "antiqua.h"

MONExternC void initAudio(void);
MONExternC u8 playAudio(void);

u8 stopAudio(void);
OSStatus resetAudio(void);

#endif

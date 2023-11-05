#ifndef osx_input_h
#define osx_input_h

#include <IOKit/hid/IOHIDLib.h>

#include "types.h"
#include "antiqua.h"

extern IOHIDManagerRef ioHIDManager;

u8 initControllerInput(void);
IOReturn resetInput(void);
void resetInputStateAll(void);
void resetAllInputStateButtons(void);

#endif

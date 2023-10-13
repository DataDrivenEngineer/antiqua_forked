#ifndef osx_input_h
#define osx_input_h

#include <IOKit/hid/IOHIDLib.h>

#include "types.h"
#include "antiqua.h"

extern IOHIDManagerRef ioHIDManager;
extern struct GameControllerInput gcInput;

u8 initControllerInput(void);
IOReturn resetInput(void);
void resetInputState(void);

#endif

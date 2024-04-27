#ifndef osx_input_h
#define osx_input_h

#include <IOKit/hid/IOHIDLib.h>

#include "types.h"

extern GameControllerInput gcInput;
extern IOHIDManagerRef ioHIDManager;

b32 initControllerInput(void);
IOReturn resetInput(void);
MONExternC RESET_INPUT_STATE_BUTTONS(resetInputStateButtons);
void resetInputStateAll(void);

#endif

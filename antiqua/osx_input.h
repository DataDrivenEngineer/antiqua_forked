#ifndef osx_input_h
#define osx_input_h

#include <IOKit/hid/IOHIDLib.h>

#include "types.h"

extern IOHIDManagerRef ioHIDManager;

u8 initControllerInput();
IOReturn resetInput();

#endif

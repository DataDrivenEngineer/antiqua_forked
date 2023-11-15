#include "osx_input.h"

#define INPUT_NOT_INITIALIZED -9999
#define DEAD_ZONE 15

IOHIDManagerRef __nullable ioHIDManager;
struct GameControllerInput gcInput;

// this will be called when the HID Manager matches a new (hot plugged) HID device
static void inIOHIDDeviceRegistrationCallback(
            void * __nullable inContext,       // context from IOHIDManagerRegisterDeviceMatchingCallback
            IOReturn inResult,        // the result of the matching operation
            void * __nullable inSender,        // the IOHIDManagerRef for the new device
            IOHIDDeviceRef __nullable inIOHIDDeviceRef // the new HID device
) {
    Boolean isGamepad = IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);
    if (isGamepad)
    {
      fprintf(stderr, "Gamepad found\n!");
      waitIfInputBlocked();
      gcInput.isAnalog = 1;
    }
}

// this will be called when a HID device is removed (unplugged)
static void inIOHIDDeviceRemovalCallback(
                void * __nullable inContext,       // context from IOHIDManagerRegisterDeviceMatchingCallback
                IOReturn inResult,        // the result of the removing operation
                void * __nullable inSender,        // the IOHIDManagerRef for the device being removed
                IOHIDDeviceRef __nullable inIOHIDDeviceRef // the removed HID device
)
{
    fprintf(stderr, "Gamepad disconnected\n!");
    waitIfInputBlocked();
    gcInput.isAnalog = 0;
}

void resetInputStateAll(void)
{
  gcInput.isAnalog = 0;
  gcInput.stickAverageX = 127.f;
  gcInput.stickAverageY = 127.f;

  for (s32 i = 0; i < ARRAY_COUNT(gcInput.buttons); i++)
  {
    gcInput.buttons[i].halfTransitionCount = INPUT_NOT_INITIALIZED;
    gcInput.buttons[i].endedDown = 0;
  }
}

MONExternC RESET_INPUT_STATE_BUTTONS(resetInputStateButtons)
{
  for (s32 i = 0; i < ARRAY_COUNT(gcInput.buttons); i++)
  {
    gcInput.buttons[i].halfTransitionCount = INPUT_NOT_INITIALIZED;
  }
}

static void inputValueCallback(void * _Nullable context, IOReturn result, void * _Nullable sender, IOHIDValueRef __nullable value)
{
  waitIfInputBlocked();

  // TODO: support controllers other than PS5 DualSense
  s32 up;
  s32 down;
  s32 left;
  s32 right;
  s32 leftBumper;
  s32 rightBumper;
  s32 leftShoulder;
  s32 rightShoulder;
  s32 squareButton; //Xbox: X
  s32 triangleButton; //Xbox: Y
  s32 circleButton; //Xbox: B
  s32 crossButton; //Xbox: A
  r32 lStickX;
  r32 lStickY;

  IOHIDElementRef elem = IOHIDValueGetElement(value);
  s16 usagePage = IOHIDElementGetUsagePage(elem);
  s16 usage = IOHIDElementGetUsage(elem);
  CFIndex intValue = IOHIDValueGetIntegerValue(value);
  if (usagePage == kHIDPage_GenericDesktop)
  {
    lStickX = usage == kHIDUsage_GD_X ? intValue : INPUT_NOT_INITIALIZED;
    lStickY = usage == kHIDUsage_GD_Y ? intValue : INPUT_NOT_INITIALIZED;
    up = (usage == kHIDUsage_GD_Hatswitch && intValue == 0) ? intValue : INPUT_NOT_INITIALIZED;
    down = (usage == kHIDUsage_GD_Hatswitch && intValue == 4) ? intValue : INPUT_NOT_INITIALIZED;
    left = (usage == kHIDUsage_GD_Hatswitch && intValue == 6) ? intValue : INPUT_NOT_INITIALIZED;
    right = (usage == kHIDUsage_GD_Hatswitch && intValue == 2) ? intValue : INPUT_NOT_INITIALIZED;

    if (lStickX != INPUT_NOT_INITIALIZED)
    {
      fprintf(stderr, "lStickX raw: %f\n", lStickX);
      if (lStickX >= 127 - DEAD_ZONE && lStickX <= 127 + DEAD_ZONE)
      {
	gcInput.stickAverageX = 127;
      }
      else
      {
	gcInput.stickAverageX = lStickX;
      }
    }
    if (lStickY != INPUT_NOT_INITIALIZED)
    {
      fprintf(stderr, "lStickY raw: %f\n", lStickY);
      if (lStickY >= 127 - DEAD_ZONE && lStickY <= 127 + DEAD_ZONE)
      {
	gcInput.stickAverageY = 127;
      }
      else
      {
	gcInput.stickAverageY = lStickY;
      }
    }
  }
  else if (usagePage == kHIDPage_Button)
  {
    leftBumper = usage == kHIDUsage_Button_5 ? intValue : INPUT_NOT_INITIALIZED;
    rightBumper = usage == kHIDUsage_Button_6 ? intValue : INPUT_NOT_INITIALIZED;
    leftShoulder = usage == kHIDUsage_Button_7 ? intValue : INPUT_NOT_INITIALIZED;
    rightShoulder = usage == kHIDUsage_Button_8 ? intValue : INPUT_NOT_INITIALIZED;
    squareButton = usage == kHIDUsage_Button_1 ? intValue : INPUT_NOT_INITIALIZED;
    triangleButton = usage == kHIDUsage_Button_4 ? intValue : INPUT_NOT_INITIALIZED;
    circleButton = usage == kHIDUsage_Button_3 ? intValue : INPUT_NOT_INITIALIZED;
    crossButton = usage == kHIDUsage_Button_2 ? intValue : INPUT_NOT_INITIALIZED;
  }
}

/**
  Populate dictionary with up to two key-value pairs
*/
static void populateMatchingDictionary(CFMutableDictionaryRef __nonnull matcher, CFStringRef __nonnull keyOne, s32 valOne, CFStringRef __nonnull keyTwo, s32 valTwo)
{
  if (matcher)
  {
    // Add key for device type to refine the matching dictionary.
    CFNumberRef valOneNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &valOne);
    if (valOneNumberRef) {
      CFDictionarySetValue(matcher, keyOne, valOneNumberRef);
      CFRelease(valOneNumberRef);

      // note: the usage is only valid if the usage page is also defined
      CFNumberRef valTwoNumberRef = CFNumberCreate(kCFAllocatorDefault, kCFNumberIntType, &valTwo);
      if (valTwoNumberRef) {
	  CFDictionarySetValue(matcher, keyTwo, valTwoNumberRef);
	  CFRelease(valTwoNumberRef);
      } else {
	fprintf(stderr, "%s: CFNumberCreate(valTwo) failed.", __PRETTY_FUNCTION__);
      }
    } else {
      fprintf(stderr, "%s: CFNumberCreate(valOne) failed.", __PRETTY_FUNCTION__);
    }
  } else {
    fprintf(stderr, "%s: CFDictionaryCreateMutable is null.", __PRETTY_FUNCTION__);
  }
}

u8 initControllerInput(void)
{
  ioHIDManager = IOHIDManagerCreate(
		  0,   // Use default allocator
		  kIOHIDOptionsTypeNone);

  // create a dictionary to only match controllers
  CFMutableDictionaryRef deviceMatcher = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  populateMatchingDictionary(deviceMatcher, CFSTR(kIOHIDDeviceUsagePageKey), kHIDPage_GenericDesktop, CFSTR(kIOHIDDeviceUsageKey), kHIDUsage_GD_GamePad);
  // Match all controllers
  IOHIDManagerSetDeviceMatching(ioHIDManager, deviceMatcher);
  CFRelease(deviceMatcher);

  // register device matching callback routine
  // this routine will be called when a new (matching) device is connected.
  IOHIDManagerRegisterDeviceMatchingCallback(
      ioHIDManager,      // HID Manager reference
      inIOHIDDeviceRegistrationCallback,  // Pointer to the callback routine
      0);             // Pointer to be passed to the callback
  // Registers a routine to be called when any currently enumerated device is removed.
  // This routine will be called when a (matching) device is disconnected.
  IOHIDManagerRegisterDeviceRemovalCallback(
      ioHIDManager,      // HID Manager reference
      inIOHIDDeviceRemovalCallback,  // Pointer to the callback routine
      0);             // Pointer to be passed to the callback

  // create a dictionary to only match the input usages from within the relevant range
  CFMutableDictionaryRef inputUsageMatcher = CFDictionaryCreateMutable(
      kCFAllocatorDefault, 0, &kCFTypeDictionaryKeyCallBacks, &kCFTypeDictionaryValueCallBacks);
  // Min and max usage values come from IOHIDUsageTables.h for Generic Desktop Usage and Buttons sections.
  s32 usageMin = 0x01;
  s32 usageMax = 0xFF;
  populateMatchingDictionary(inputUsageMatcher, CFSTR(kIOHIDElementUsageMinKey), usageMin, CFSTR(kIOHIDElementUsageMaxKey), usageMax);
  IOHIDManagerSetInputValueMatching(ioHIDManager, inputUsageMatcher);
  CFRelease(inputUsageMatcher);

  IOHIDManagerRegisterInputValueCallback(ioHIDManager, inputValueCallback, 0);

  IOHIDManagerScheduleWithRunLoop(ioHIDManager, CFRunLoopGetCurrent(), kCFRunLoopDefaultMode);

  // Open a HID Manager reference
  IOReturn ioResult = IOHIDManagerOpen(
	  ioHIDManager,  // HID Manager reference
	  kIOHIDOptionsTypeNone);         // Option bits
  if (ioResult != kIOReturnSuccess)
  {
    fprintf(stderr, "Failed to open HIDManager, error: %d\n", ioResult);
    return ioResult;
  }
  return kIOReturnSuccess;
}

IOReturn resetInput(void)
{
  IOReturn ioResult = IOHIDManagerClose(
	  ioHIDManager,  // HID Manager reference
	  kIOHIDOptionsTypeNone);         // Option bits
  if (ioResult != kIOReturnSuccess)
  {
    fprintf(stderr, "Failed to close HIDManager, error: %d\n", ioResult);
    return ioResult;
  }

  CFRelease(ioHIDManager);
  return kIOReturnSuccess;
}

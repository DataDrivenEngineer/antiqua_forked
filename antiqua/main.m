//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <IOKit/hid/IOHIDLib.h>

#import "CustomNSView.h"
#import "CustomWindowDelegate.h"
#import "CustomCALayer.h"
#import "AppDelegate.h"

#include "types.h"
#include "osx_audio.h"
#include "antiqua.cpp"

static u8 shouldKeepRunning = 1;

static IOHIDManagerRef ioHIDManager;


static void processEvent(NSEvent *e)
{
  if (e.type == NSEventTypeKeyDown)
  {
    if (e.keyCode == kVK_Escape)
    {
      shouldKeepRunning = 0;
    }
    if (e.keyCode == kVK_ANSI_S)
    {
      incYOff(10);
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      incXOff(10);
    }
  }
}

// this will be called when the HID Manager matches a new (hot plugged) HID device
static void inIOHIDDeviceRegistrationCallback(
            void *inContext,       // context from IOHIDManagerRegisterDeviceMatchingCallback
            IOReturn inResult,        // the result of the matching operation
            void *inSender,        // the IOHIDManagerRef for the new device
            IOHIDDeviceRef inIOHIDDeviceRef // the new HID device
) {
    Boolean isGamepad = IOHIDDeviceConformsTo(inIOHIDDeviceRef, kHIDPage_GenericDesktop, kHIDUsage_GD_GamePad);
    if (isGamepad)
    {
      fprintf(stderr, "Gamepad found\n!");
    }
}

// this will be called when a HID device is removed (unplugged)
static void inIOHIDDeviceRemovalCallback(
                void *inContext,       // context from IOHIDManagerRegisterDeviceMatchingCallback
                IOReturn inResult,        // the result of the removing operation
                void *inSender,        // the IOHIDManagerRef for the device being removed
                IOHIDDeviceRef inIOHIDDeviceRef // the removed HID device
) {
    printf("%s(context: %p, result: %p, sender: %p, device: %p).\n",
        __PRETTY_FUNCTION__, inContext, (void *) inResult, inSender, (void*) inIOHIDDeviceRef);
}

static void inputValueCallback(void * _Nullable context, IOReturn result, void * _Nullable sender, IOHIDValueRef value)
{
  // TODO: support controllers other than PS5 DualSense
  u8 up;
  u8 down;
  u8 left;
  u8 right;
  u8 leftBumper;
  u8 rightBumper;
  u8 leftShoulder;
  u8 rightShoulder;
  u8 squareButton; //Xbox: X
  u8 triangleButton; //Xbox: Y
  u8 circleButton; //Xbox: B
  u8 crossButton; //Xbox: A
  s16 lStickX;
  s16 lStickY;

  IOHIDElementRef elem = IOHIDValueGetElement(value);
  s16 usagePage = IOHIDElementGetUsagePage(elem);
  s16 usage = IOHIDElementGetUsage(elem);
  CFIndex intValue = IOHIDValueGetIntegerValue(value);
  if (usagePage == kHIDPage_GenericDesktop)
  {
    lStickX = usage == kHIDUsage_GD_X ? intValue : 0;
    lStickY = usage == kHIDUsage_GD_Y ? intValue : 0;
    u8 up = (usage == kHIDUsage_GD_Hatswitch && intValue == 0) ? intValue : 0;
    u8 down = (usage == kHIDUsage_GD_Hatswitch && intValue == 4) ? intValue : 0;
    u8 left = (usage == kHIDUsage_GD_Hatswitch && intValue == 6) ? intValue : 0;
    u8 right = (usage == kHIDUsage_GD_Hatswitch && intValue == 2) ? intValue : 0;

    if (lStickX)
    {
      s16 normalized = (s16) ((lStickX / 256.f) * (32767 + 32767) - 32767);
      fprintf(stderr, "%d\n", normalized);
      incXOff(normalized >> 12);
    }
    if (lStickY)
    {
      s16 normalized = (s16) ((lStickY / 255.f) * (32767 + 32767) - 32767);
      s32 toneHzModifier = (s32) (256.f * (normalized / 32767.f));
      // TODO: move this to platform-independent code
//      soundState.toneHz = 512 + toneHzModifier;
      incYOff(normalized >> 12);
    }
  }
  else if (usagePage == kHIDPage_Button)
  {
    u8 leftBumper = usage == kHIDUsage_Button_5 ? intValue : 0;
    u8 rightBumper = usage == kHIDUsage_Button_6 ? intValue : 0;
    u8 leftShoulder = usage == kHIDUsage_Button_7 ? intValue : 0;
    u8 rightShoulder = usage == kHIDUsage_Button_8 ? intValue : 0;
    u8 squareButton = usage == kHIDUsage_Button_1 ? intValue : 0;
    u8 triangleButton = usage == kHIDUsage_Button_4 ? intValue : 0;
    u8 circleButton = usage == kHIDUsage_Button_3 ? intValue : 0;
    u8 crossButton = usage == kHIDUsage_Button_2 ? intValue : 0;
  }
}

/**
  Populate dictionary with up to two key-value pairs
*/
static void populateMatchingDictionary(CFMutableDictionaryRef matcher, CFStringRef keyOne, s32 valOne, CFStringRef keyTwo, s32 valTwo)
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

static u8 initControllerInput()
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
    return -1;
  }
}

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    // Setup code that might create autoreleased objects goes here.
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];
    id appDelegate = [AppDelegate alloc];
    [NSApp setDelegate:appDelegate];

    id menuBar = [NSMenu new];
    id appMenuItem = [NSMenuItem new];
    [menuBar addItem:appMenuItem];
    [NSApp setMainMenu:menuBar];
    id appMenu = [NSMenu new];
    id appName = [[NSProcessInfo processInfo] processName];
    id quitTitle = @"Quit";
    id quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
    action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 600)
    styleMask:NSTitledWindowMask | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
    backing:NSBackingStoreBuffered defer:NO];
    // Center the window
    CGFloat xPos = NSWidth([[window screen] frame]) / 2 - NSWidth([window frame]) / 2;
    CGFloat yPos = NSHeight([[window screen] frame]) / 2 + NSHeight([window frame]) / 2;
    [window cascadeTopLeftFromPoint:NSMakePoint(xPos,yPos)];
    [window setTitle:@"Antiqua"];
    [window makeKeyAndOrderFront:nil];

    CustomNSView *view = [[CustomNSView alloc]initWithFrame:[window frame]];
    window.contentView = view;
    
    CustomWindowDelegate *delegate = [[CustomWindowDelegate alloc] init];
    window.delegate = delegate;

    [NSApp activateIgnoringOtherApps:YES];
    [NSApp finishLaunching];

    s32 result = initControllerInput();
    if (result)
    {
      fprintf(stderr, "Failed to initialize controller input!\n");
      return result;
    }

    while (shouldKeepRunning)
    {
      @autoreleasepool
      {
	NSEvent *event = [NSApp
	  nextEventMatchingMask:NSAnyEventMask
	  untilDate:[NSDate distantPast]
	  inMode:NSDefaultRunLoopMode
	  dequeue:YES];

	processEvent(event);

	[NSApp sendEvent:event];
	[NSApp updateWindows];
      }
    }

    // As part of cleanup, reset the audio hardware to the state we found it in
    resetAudio();

    IOReturn ioResult = IOHIDManagerClose(
            ioHIDManager,  // HID Manager reference
            kIOHIDOptionsTypeNone);         // Option bits
    if (ioResult != kIOReturnSuccess)
    {
      fprintf(stderr, "Failed to close HIDManager, error: %d\n", ioResult);
      return -1;
    }
    CFRelease(ioHIDManager);

    [NSApp terminate:NSApp];
  }
  return 0;
}

//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <CoreAudio/AudioHardware.h>

#import "CustomNSView.h"
#import "CustomWindowDelegate.h"
#import "CustomCALayer.h"
#import "CustomNSView.h"
#import "AppDelegate.h"

#include "types.h"
#include "antiqua.cpp"

static u8 shouldKeepRunning = 1;

static struct SoundState soundState = {0};
static AudioObjectID device = kAudioObjectUnknown;
static AudioDeviceIOProcID procID;
static u8 soundPlaying = 0;

static OSStatus appIOProc(AudioObjectID inDevice,
                        const AudioTimeStamp*   inNow,
                        const AudioBufferList*  inInputData,
                        const AudioTimeStamp*   inInputTime,
                        AudioBufferList*        outOutputData,
                        const AudioTimeStamp*   inOutputTime,
                        void* __nullable        inClientData)
{
  struct SoundState *soundState = (struct SoundState *) inClientData;
  soundState->frames = (r32 *) outOutputData->mBuffers[0].mData;
  fillSoundBuffer(soundState);

  return kAudioHardwareNoError;     
}

static u8 playAudio()
{
  OSStatus err = kAudioHardwareNoError;

  if (soundPlaying) return 0;
  
  err = AudioDeviceCreateIOProcID(device, appIOProc, (void *) &soundState, &procID);
  if (err != kAudioHardwareNoError) return 0;

  err = AudioDeviceStart(device, procID);				// start playing sound through the device
  if (err != kAudioHardwareNoError) return 0;

  soundPlaying = 1; // set the playing status global to true
  return 1;
}

static u8 stopAudio()
{
    OSStatus err = kAudioHardwareNoError;
    
    if (!soundPlaying) return 0;
    
    err = AudioDeviceStop(device, procID);				// stop playing sound through the device
    if (err != kAudioHardwareNoError) 
    {
      if (err == kAudioHardwareBadDeviceError)
      {
	// Device not found, could be because e.g. headphones got unplugged - still must set sound to not playing
	soundPlaying = 0;
      }
      return 0;
    }

    err = AudioDeviceDestroyIOProcID(device, procID);
    if (err != kAudioHardwareNoError) return 0;

    soundPlaying = 0;						// set the playing status global to false
    return 1;
}

OSStatus appObjPropertyListenerProc(AudioObjectID nObjectID, UInt32 inNumberAddresses, const AudioObjectPropertyAddress* inAddresses, void* __nullable inClientData)
{
  for (int i = 0; i < inNumberAddresses; i++)
  {
    if (inAddresses[i].mSelector == kAudioHardwarePropertyDefaultOutputDevice)
    {
      fprintf(stderr, "Default audio device changed!\n");
      stopAudio();
    }
  }
  return kAudioHardwareNoError;     
}

void initAudio(void)
{
  OSStatus err = kAudioHardwareNoError;
  u32 tempSize;
  AudioObjectPropertyAddress tempPropertyAddress = {0};
  AudioObjectPropertyAddress devicePropertyAddress = {0};
  u32 newBufferSize;

  soundState.sampleRate = 48000;
  soundState.toneHz = 256;
  // Num of frames enough for playing audio for 1/30th of a second
  soundState.needFrames = soundState.sampleRate / 30;
  // Each frame has 2 samples, each sample is a float (4 bytes)
  newBufferSize = soundState.needFrames * 2 * 4;
 
  // get the default output device for the HAL
  devicePropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  devicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
  devicePropertyAddress.mElement = kAudioObjectPropertyElementMain;
  tempSize = sizeof(device);		// it is required to pass the size of the data to be returned
  err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicePropertyAddress, 0, 0, &tempSize, (void *) &device);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "get kAudioHardwarePropertyDefaultOutputDevice error %ld\n", err);
      return;
  }

  // tell the HAL to manage its own thread for notifications. Need this so that the listener for audio default device change is called
  AudioObjectPropertyAddress loopPropertyAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
  if(!AudioObjectHasProperty(kAudioObjectSystemObject, &loopPropertyAddress))
  {
    CFRunLoopRef theRunLoop = 0;
    err = AudioObjectSetPropertyData(kAudioObjectSystemObject, &loopPropertyAddress, 0, 0, sizeof(CFRunLoopRef), &theRunLoop);
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "failed to register run loop, error: %ld\n", err);
	return;
    }
  }

  err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &devicePropertyAddress, appObjPropertyListenerProc, 0);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to add listener for kAudioHardwarePropertyDefaultOutputDevice property, error: %ld\n", err);
  }

  tempPropertyAddress.mSelector = kAudioDevicePropertyBufferSize;
  tempPropertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  tempPropertyAddress.mElement = kAudioObjectPropertyElementMain;
  err = AudioObjectSetPropertyData(device, &tempPropertyAddress, 0, 0, sizeof(newBufferSize), &newBufferSize);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to update buffer size, error: %ld\n", err);
      return;
  }

  AudioStreamBasicDescription audioDataFormat = {0};
  tempSize = sizeof(audioDataFormat);				// it is required to pass the size of the data to be returned
  tempPropertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
  tempPropertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  tempPropertyAddress.mElement = kAudioObjectPropertyElementMain;
  // Values below are for stereo 
  audioDataFormat.mSampleRate = 48000.f;
  audioDataFormat.mFormatID = kAudioFormatLinearPCM;
  audioDataFormat.mFormatFlags = kAudioFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
  audioDataFormat.mBitsPerChannel = 32;
  audioDataFormat.mBytesPerFrame = 8;
  audioDataFormat.mChannelsPerFrame = 2;
  audioDataFormat.mBytesPerPacket = 8;
  audioDataFormat.mFramesPerPacket = 1;

  err = AudioObjectSetPropertyData(device, &tempPropertyAddress, 0, 0, tempSize, &audioDataFormat);
  if (err != kAudioHardwareNoError)
  {
      fprintf(stderr, "set kAudioDevicePropertyStreamFormat error %ld\n", err);
  }
}


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
      incYOff();
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      incXOff();
    }
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

	if (!soundPlaying)
	{
	  initAudio();
	  soundPlaying = playAudio();
	}

	[NSApp sendEvent:event];
	[NSApp updateWindows];
      }
    }

    // As part of cleanup, reset the audio hardware to the state we found it in
    OSStatus err = kAudioHardwareNoError;
    err = AudioHardwareUnload();
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "failed to unload hardware, error: %ld\n", err);
    }

    [NSApp terminate:NSApp];
  }
  return 0;
}

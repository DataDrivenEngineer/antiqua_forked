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
#include "antiqua.h"
#include "antiqua.cpp"

static u8 shouldKeepRunning = 1;

static struct SoundState soundState = {0};
static AudioObjectID device = kAudioObjectUnknown;
static u8 soundPlaying = 0;

static OSStatus appIOProc(AudioObjectID inDevice,
                        const AudioTimeStamp*   inNow,
                        const AudioBufferList*  inInputData,
                        const AudioTimeStamp*   inInputTime,
                        AudioBufferList*        outOutputData,
                        const AudioTimeStamp*   inOutputTime,
                        void* __nullable        inClientData)
{
  fprintf(stderr, "num of buffers in list: %d\n", outOutputData->mNumberBuffers);
  fprintf(stderr, "size of audio buffer: %d\n", outOutputData->mBuffers[0].mDataByteSize);
  fprintf(stderr, "num of interleaved channels in audio buffer: %d\n", outOutputData->mBuffers[0].mNumberChannels);
  struct SoundState *soundState = (struct SoundState *) inClientData;
  fprintf(stderr, "soundState needFrames: %d\n", soundState->needFrames);
  soundState->frames = (r32 *) outOutputData->mBuffers[0].mData;
  fillSoundBuffer(soundState);

  return kAudioHardwareNoError;     
}

static u8 playAudio()
{
  OSStatus err = kAudioHardwareNoError;

  if (soundPlaying) return 0;
  
  AudioDeviceIOProcID procID;
  err = AudioDeviceCreateIOProcID(device, appIOProc, (void *) &soundState, &procID);
  if (err != kAudioHardwareNoError) return 0;

  err = AudioDeviceStart(device, procID);				// start playing sound through the device
  if (err != kAudioHardwareNoError) return 0;

  soundPlaying = 1; // set the playing status global to true
  return soundPlaying;
}

static u8 stopAudio()
{
    OSStatus err = kAudioHardwareNoError;
    
    if (!soundPlaying) return 0;
    
    err = AudioDeviceStop(device, appIOProc);				// stop playing sound through the device
    if (err != kAudioHardwareNoError) return 0;

    err = AudioDeviceRemoveIOProc(device, appIOProc);			// remove the IO proc from the device
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
//      OSStatus err = kAudioHardwareNoError;
//      err = AudioHardwareUnload();
//      if (err != kAudioHardwareNoError) {
//	  fprintf(stderr, "failed to unload hardware, error: %ld\n", err);
//      }
      void initAudio(void);
      initAudio();
      playAudio();
    }
  }
  return kAudioHardwareNoError;     
}

void initAudio(void)
{
  OSStatus err = kAudioHardwareNoError;
  r32 volumeScalar;
  u32 count;
  AudioObjectPropertyAddress devicePropertyAddress = {0};
  AudioObjectPropertyAddress propertyAddress = {0};
  AudioStreamBasicDescription deviceFormat;
  AudioBufferList bufferList;
  u32 deviceBufferSize;
 
  // get the default output device for the HAL
  devicePropertyAddress.mSelector = kAudioHardwarePropertyDefaultOutputDevice;
  devicePropertyAddress.mScope = kAudioObjectPropertyScopeGlobal;
  devicePropertyAddress.mElement = kAudioObjectPropertyElementMain;
  count = sizeof(device);		// it is required to pass the size of the data to be returned
  err = AudioObjectGetPropertyData(kAudioObjectSystemObject, &devicePropertyAddress, 0, 0, &count, (void *) &device);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "get kAudioHardwarePropertyDefaultOutputDevice error %ld\n", err);
      return;
  }

  AudioObjectPropertyAddress loopPropertyAddress = { kAudioHardwarePropertyRunLoop, kAudioObjectPropertyScopeGlobal, kAudioObjectPropertyElementMain };
  // tell the HAL to manage its own thread for notifications. Need this so that the listener for audio default change is called
  if(!AudioObjectHasProperty(kAudioObjectSystemObject, &loopPropertyAddress))
  {
    CFRunLoopRef theRunLoop = 0;
    err = AudioObjectSetPropertyData(kAudioObjectSystemObject, &loopPropertyAddress, 0, 0, sizeof(CFRunLoopRef), &theRunLoop);
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "failed to register run loop, error: %ld\n", err);
	return;
    }
  }

  if(!AudioObjectHasProperty(kAudioObjectSystemObject, &devicePropertyAddress))
  {
    err = AudioObjectAddPropertyListener(kAudioObjectSystemObject, &devicePropertyAddress, appObjPropertyListenerProc, 0);
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "failed to add listener for kAudioHardwarePropertyDefaultOutputDevice property, error: %ld\n", err);
	return;
    }
  }

  // get the bufferlist that the default device uses for IO
  count = sizeof(bufferList);	// it is required to pass the size of the data to be returned
  propertyAddress.mSelector = kAudioDevicePropertyStreamConfiguration;
  propertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  propertyAddress.mElement = kAudioObjectPropertyElementMain;
  err = AudioObjectGetPropertyData(device, &propertyAddress, 0, 0, &count, (void *) &bufferList);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "get kAudioDevicePropertyStreamConfiguration error %ld\n", err);
      return;
  }
  fprintf(stderr, "bufferList = %ld\n", bufferList);

  // get the buffersize that the default device uses for IO
  count = sizeof(deviceBufferSize);	// it is required to pass the size of the data to be returned
  propertyAddress.mSelector = kAudioDevicePropertyBufferSize;
  propertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  propertyAddress.mElement = kAudioObjectPropertyElementMain;
  err = AudioObjectGetPropertyData(device, &propertyAddress, 0, 0, &count, (void *) &deviceBufferSize);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "get kAudioDevicePropertyBufferSize error %ld\n", err);
      return;
  }
  fprintf(stderr, "deviceBufferSize = %ld\n", deviceBufferSize);
  
  u32 newBufferSize = 48000 / 30 * 2 * 4;
  err = AudioObjectSetPropertyData(device, &propertyAddress, 0, 0, sizeof(u32), &newBufferSize);
  if (err != kAudioHardwareNoError) {
      fprintf(stderr, "failed to update buffer size, error: %ld\n", err);
      return;
  }

  // get a description of the data format used by the default device
//  count = sizeof(deviceFormat);				// it is required to pass the size of the data to be returned
//  propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
//  propertyAddress.mScope = kAudioObjectPropertyScopeOutput;
//  propertyAddress.mElement = kAudioObjectPropertyElementMain;
//  err = AudioObjectGetPropertyData(device, &propertyAddress, 0, 0, &count, (void *) &deviceFormat);
//  if (err != kAudioHardwareNoError)
//  {
//      fprintf(stderr, "get kAudioDevicePropertyStreamFormat error %ld\n", err);
//  }
//  fprintf(stderr, "old format:\n");
//  fprintf(stderr, "kAudioDevicePropertyStreamFormat %d\n", err);
//  fprintf(stderr, "sampleRate %g\n", deviceFormat.mSampleRate);
//  fprintf(stderr, "mFormatID %08X\n", deviceFormat.mFormatID);
//  fprintf(stderr, "mFormatFlags %08X\n", deviceFormat.mFormatFlags);
//  fprintf(stderr, "mBytesPerPacket %d\n", deviceFormat.mBytesPerPacket);
//  fprintf(stderr, "mFramesPerPacket %d\n", deviceFormat.mFramesPerPacket);
//  fprintf(stderr, "mChannelsPerFrame %d\n", deviceFormat.mChannelsPerFrame);
//  fprintf(stderr, "mBytesPerFrame %d\n", deviceFormat.mBytesPerFrame);
//  fprintf(stderr, "mBitsPerChannel %d\n", deviceFormat.mBitsPerChannel);

  AudioStreamBasicDescription audioDataFormat = {0};
  count = sizeof(audioDataFormat);				// it is required to pass the size of the data to be returned
  propertyAddress.mSelector = kAudioDevicePropertyStreamFormat;
  propertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  propertyAddress.mElement = kAudioObjectPropertyElementMain;
  // Values below are for stereo 
  audioDataFormat.mSampleRate = 48000.f;
  audioDataFormat.mFormatID = kAudioFormatLinearPCM;
  audioDataFormat.mFormatFlags = kAudioFormatFlagIsFloat | kLinearPCMFormatFlagIsPacked;
  audioDataFormat.mBitsPerChannel = 32;
  audioDataFormat.mBytesPerFrame = 8;
  audioDataFormat.mChannelsPerFrame = 2;
  audioDataFormat.mBytesPerPacket = 8;
  audioDataFormat.mFramesPerPacket = 1;

  fprintf(stderr, "new format:\n");
  fprintf(stderr, "kAudioDevicePropertyStreamFormat %d\n", err);
  fprintf(stderr, "sampleRate %g\n", audioDataFormat.mSampleRate);
  fprintf(stderr, "mFormatID %08X\n", audioDataFormat.mFormatID);
  fprintf(stderr, "mFormatFlags %08X\n", audioDataFormat.mFormatFlags);
  fprintf(stderr, "mBytesPerPacket %d\n", audioDataFormat.mBytesPerPacket);
  fprintf(stderr, "mFramesPerPacket %d\n", audioDataFormat.mFramesPerPacket);
  fprintf(stderr, "mChannelsPerFrame %d\n", audioDataFormat.mChannelsPerFrame);
  fprintf(stderr, "mBytesPerFrame %d\n", audioDataFormat.mBytesPerFrame);
  fprintf(stderr, "mBitsPerChannel %d\n", audioDataFormat.mBitsPerChannel);
  
  err = AudioObjectSetPropertyData(device, &propertyAddress, 0, 0, count, &audioDataFormat);
  if (err != kAudioHardwareNoError)
  {
      fprintf(stderr, "set kAudioDevicePropertyStreamFormat error %ld\n", err);
  }

  // Get volume scalar
  propertyAddress.mSelector = kAudioDevicePropertyVolumeScalar;
  propertyAddress.mScope = kAudioObjectPropertyScopeOutput;
  propertyAddress.mElement = kAudioObjectPropertyElementMain;
  if(AudioObjectHasProperty(device, &propertyAddress))
  {
    err = AudioObjectGetPropertyData(device, &propertyAddress, 0, 0, &count, (void *) &volumeScalar);
    if (err != kAudioHardwareNoError) {
	fprintf(stderr, "get kAudioDevicePropertyVolumeScalar error %ld\n", err);
	return;
    }
    fprintf(stderr, "old volumeScalar = %f\n", volumeScalar);
  }

  // Update volume scalar
//  volumeScalar = 0.1f;
//  err = AudioObjectSetPropertyData(device, &propertyAddress, 0, 0, count, &volumeScalar);
//  if (err != kAudioHardwareNoError)
//  {
//      fprintf(stderr, "set kAudioDevicePropertyVolumeScalar error %ld\n", err);
//  }

  soundState.sampleRate = 48000;
  soundState.toneHz = 256;
  soundState.needFrames = newBufferSize / 8;
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
    [NSApp terminate:NSApp];
  }
  return 0;
}

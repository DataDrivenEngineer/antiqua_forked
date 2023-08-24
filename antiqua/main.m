//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <AudioToolbox/AudioToolbox.h>

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
static AudioQueueRef audioQueue = 0;
static AudioQueueBufferRef audioBuffer[2] = {};

  void audioCallback(void *inUserData, AudioQueueRef inAQ, AudioQueueBufferRef inBuffer)
  {
  struct SoundState *soundState = (struct SoundState *) inUserData;
  soundState->buf = inBuffer->mAudioData;
  fillSoundBuffer(soundState);

  inBuffer->mAudioDataByteSize = inBuffer->mAudioDataBytesCapacity;

  AudioQueueEnqueueBuffer(audioQueue, inBuffer, 0, NULL);
}

void initAudio(void)
{
  soundState.sampleRate = 48000.f;
  soundState.toneHz = 256.f;
  soundState.volume = 3000.f;

  soundState.runningFrameIndex = 0;

  AudioStreamBasicDescription audioDataFormat = {0};
  // Values below are for stereo 
  audioDataFormat.mSampleRate = soundState.sampleRate;
  audioDataFormat.mFormatID = kAudioFormatLinearPCM;
  audioDataFormat.mFormatFlags = kLinearPCMFormatFlagIsBigEndian | kLinearPCMFormatFlagIsSignedInteger | kLinearPCMFormatFlagIsPacked;
  audioDataFormat.mBitsPerChannel = 16;
  audioDataFormat.mBytesPerFrame = 4;
  audioDataFormat.mChannelsPerFrame = 2;
  audioDataFormat.mBytesPerPacket = 4;
  audioDataFormat.mFramesPerPacket = 1;

  soundState.bufCapacity = audioDataFormat.mSampleRate * sizeof(s16) * 2;

  OSStatus res = AudioQueueNewOutput(
    &audioDataFormat, 
    &audioCallback, 
    &soundState, 
    0, 
    0, 
    0, 
    &audioQueue);
  if (!res)
  {
    // Allocate buffer for 2 seconds of sound
    u32 audioBufferSize = audioDataFormat.mSampleRate * sizeof(s16) * 2;
    res = AudioQueueAllocateBuffer(audioQueue, audioBufferSize, &(audioBuffer[0])) | AudioQueueAllocateBuffer(audioQueue, audioBufferSize, &(audioBuffer[1]));
    if (res)
    {
      NSLog(@"Failed to allocate audio buffers, error code: %d", res);
    }
  }
  else
  {
    NSLog(@"Failed to create audio queue, error code: %d", res);
  }
}

void playAudio(void)
{
  audioCallback(&soundState, audioQueue, audioBuffer[0]);
  audioCallback(&soundState, audioQueue, audioBuffer[1]);
  OSStatus res = AudioQueueEnqueueBuffer(audioQueue, audioBuffer[0], 0, NULL) | AudioQueueEnqueueBuffer(audioQueue, audioBuffer[1], 0, NULL);
//  OSStatus res = AudioQueueEnqueueBuffer(audioQueue, audioBuffer[0], 0, NULL); 
  if (!res)
  {
    res = AudioQueuePrime(audioQueue, 0, NULL);
    if (!res)
    {
      res = AudioQueueStart(audioQueue, NULL);
      if (res)
      {
	NSLog(@"Failed to play audio queue, error code: %d", res);
      }
    }
    else
    {
      NSLog(@"Failed to prime audio buffer, error code: %d", res);
    }
  }
  else
  {
    NSLog(@"Failed to enqueue audio buffers, error code: %d", res);
  }
}

void processEvent(NSEvent *e)
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

    initAudio();
    u8 isAudioPlaying = 0;

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

	if (!isAudioPlaying)
	{
	  isAudioPlaying = 1;
	  playAudio();
	}

	[NSApp sendEvent:event];
	[NSApp updateWindows];
      }
    }
    [NSApp terminate:NSApp];
    
    OSStatus res = AudioQueueDispose(audioQueue, false);
    if (res)
    {
      NSLog(@"Failed to dispose of audio queue, error code: %d", res);
    }
    res = AudioQueueFreeBuffer(audioQueue, audioBuffer[0]) | AudioQueueFreeBuffer(audioQueue, audioBuffer[1]);
    if (res)
    {
      NSLog(@"Failed to free audio buffers, error code: %d", res);
    }
  }
  return 0;
}

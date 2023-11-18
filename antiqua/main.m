//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>
#import <fcntl.h>
#import <sys/stat.h>
#import <unistd.h>
#import <sys/types.h>
#import <sys/uio.h>
#import <sys/mman.h>

#import "CustomNSView.h"
#import "CustomWindowDelegate.h"
#import "CustomCALayer.h"

#include "types.h"
#include "antiqua.h"
#include "osx_audio.h"
#include "osx_input.h"
#include "osx_lock.h"

static u8 shouldKeepRunning = 1;

static u8 processEvent(NSEvent *e)
{
  u8 handled = 0;
  if (e.type == NSEventTypeKeyDown)
  {
    if (e.keyCode == kVK_Escape)
    {
      handled = 1;
      shouldKeepRunning = 0;
    }
    if (e.keyCode == kVK_ANSI_S)
    {
      handled = 1;
      if (!gcInput.down.endedDown)
      {
	waitIfInputBlocked();
	gcInput.down.halfTransitionCount++;
	gcInput.down.endedDown = 1;
      }
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      handled = 1;
      if (!gcInput.right.endedDown)
      {
	waitIfInputBlocked();
	gcInput.right.halfTransitionCount++;
	gcInput.right.endedDown = 1;
      }
    }
    if (e.keyCode == kVK_ANSI_A)
    {
      handled = 1;
      if (!gcInput.left.endedDown)
      {
	waitIfInputBlocked();
	gcInput.left.halfTransitionCount++;
	gcInput.left.endedDown = 1;
      }
    }
    if (e.keyCode == kVK_ANSI_W)
    {
      handled = 1;
      if (!gcInput.up.endedDown)
      {
	waitIfInputBlocked();
	gcInput.up.halfTransitionCount++;
	gcInput.up.endedDown = 1;
      }
    }
    if (e.keyCode == kVK_ANSI_L)
    {
      handled = 1;
      if (state.inputRecordingIndex == 0)
      {
	beginRecordingInput(&state, 1);
      }
      else
      {
	endRecordingInput(&state);
	beginInputPlayBack(&state, 1);
      }
    }
  }
  else if (e.type == NSEventTypeKeyUp)
  {
    if (e.keyCode == kVK_ANSI_S)
    {
      waitIfInputBlocked();
      handled = 1;
      gcInput.down.halfTransitionCount++;
      gcInput.down.endedDown = 0;
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      waitIfInputBlocked();
      handled = 1;
      gcInput.right.halfTransitionCount++;
      gcInput.right.endedDown = 0;
    }
    if (e.keyCode == kVK_ANSI_A)
    {
      waitIfInputBlocked();
      handled = 1;
      gcInput.left.halfTransitionCount++;
      gcInput.left.endedDown = 0;
    }
    if (e.keyCode == kVK_ANSI_W)
    {
      waitIfInputBlocked();
      handled = 1;
      gcInput.up.halfTransitionCount++;
      gcInput.up.endedDown = 0;
    }
  }

  return handled;
}

inline static u32 safeTruncateUInt64(u64 value)
{
  ASSERT(value <= 0xFFFFFFFF);
  u32 result = (u32) value;
  return result;
}

MONExternC DEBUG_PLATFORM_FREE_FILE_MEMORY(debug_platformFreeFileMemory)
{
  msync(file->contents, file->contentsSize, MS_SYNC);
  munmap(file->contents, file->contentsSize);
}

MONExternC DEBUG_PLATFORM_READ_ENTIRE_FILE(debug_platformReadEntireFile)
{
  u8 result = 0;
  s32 fd = open(filename, O_RDONLY | O_SHLOCK);
  if (fd != -1)
  {
    struct stat st;
    if (fstat(fd, &st) != -1)
    {
      outFile->contentsSize = safeTruncateUInt64(st.st_size);
      outFile->contents = mmap(0, outFile->contentsSize, PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
      msync(outFile->contents, outFile->contentsSize, MS_SYNC | MS_INVALIDATE);
      if (outFile->contents != MAP_FAILED)
      {
	s32 bytesRead = read(fd, outFile->contents, outFile->contentsSize);
	if (bytesRead != -1 && bytesRead == outFile->contentsSize)
	{
	  // TODO: File read successfully. Now do something with it...
	  result = 1;
	}
	else
	{
	  fprintf(stderr, "Failed to read from a file %s, bytesRead: %d, error: %d\n", filename, bytesRead, errno);
	  debug_platformFreeFileMemory(outFile);
	  result = 0;
	}
      }
      else
      {
	fprintf(stderr, "Failed to allocate memory for a file %s - error: %d\n", filename, errno);
      }
    }
    else
    {
      fprintf(stderr, "Failed to get stats for a file %s - error: %d\n", filename, errno);
    }

    if (close(fd) == -1)
    {
      fprintf(stderr, "Failed to close a file %s, error: %d\n", filename, errno);
    }
  }
  else
  {
    fprintf(stderr, "Failed to open a file %s, error: %d\n", filename, errno);
  }

  return result;
}

MONExternC DEBUG_PLATFORM_WRITE_ENTIRE_FILE(debug_platformWriteEntireFile)
{
  u8 result = 0;

  s32 fd = open(filename, O_WRONLY | O_EXLOCK | O_CREAT, 0777);
  if (fd != -1)
  {
    if (write(fd, memory, memorySize) != -1)
    {
      result = 1;
    }
    else
    {
      fprintf(stderr, "Failed to write to a file %s, error: %d\n", filename, errno);
    }
  }
  else
  {
    fprintf(stderr, "Failed to open a file %s, error: %d\n", filename, errno);
  }

  return result;
}

int main(int argc, const char * argv[]) {
  @autoreleasepool {
    // Setup code that might create autoreleased objects goes here.
    [NSApplication sharedApplication];
    [NSApp setActivationPolicy:NSApplicationActivationPolicyRegular];

    id menuBar = [NSMenu new];
    id appMenuItem = [NSMenuItem new];
    [menuBar addItem:appMenuItem];
    [NSApp setMainMenu:menuBar];
    id appMenu = [NSMenu new];
    id quitTitle = @"Quit";
    id quitMenuItem = [[NSMenuItem alloc] initWithTitle:quitTitle
    action:@selector(terminate:) keyEquivalent:@"q"];
    [appMenu addItem:quitMenuItem];
    [appMenuItem setSubmenu:appMenu];

#if !ANTIQUA_INTERNAL
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 600)
    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable | NSWindowStyleMaskNonactivatingPanel
    backing:NSBackingStoreBuffered defer:NO];
    [window setCollectionBehavior: NSWindowCollectionBehaviorMoveToActiveSpace | NSWindowCollectionBehaviorFullScreenPrimary];
    // Uncomment the code below for looped live code editing
//    [window setFloatingPanel:YES];
//    [window setCollectionBehavior: NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary];
//    [window setLevel: NSStatusWindowLevel];
#else
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 600)
    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
    backing:NSBackingStoreBuffered defer:NO];
    [window setCollectionBehavior: NSWindowCollectionBehaviorMoveToActiveSpace | NSWindowCollectionBehaviorFullScreenPrimary];
#endif

    // Center the window
    CGFloat xPos = NSWidth([[window screen] frame]) / 2 - NSWidth([window frame]) / 2;
    CGFloat yPos = NSHeight([[window screen] frame]) / 2 + NSHeight([window frame]) / 2;
    [window makeKeyAndOrderFront:nil];
    [window cascadeTopLeftFromPoint:NSMakePoint(xPos,yPos)];
    [window setTitle:@"Antiqua"];

    CustomNSView *view = [[CustomNSView alloc]initWithFrame:[window frame]];
    window.contentView = view;
    
    CustomWindowDelegate *delegate = [[CustomWindowDelegate alloc] init];
    window.delegate = delegate;

    [NSApp activateIgnoringOtherApps:YES];
    [NSApp finishLaunching];

    resetInputStateAll();
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
	if (!soundState.soundPlaying)
	{
	  initAudio();
	  soundState.soundPlaying = playAudio();
	}

	NSEvent *event = [NSApp
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	  nextEventMatchingMask:NSAnyEventMask
#pragma clang diagnostic pop
	  untilDate:[NSDate distantPast]
	  inMode:NSDefaultRunLoopMode
	  dequeue:YES];

	u8 handled = processEvent(event);

	if (!handled)
	{
	  [NSApp sendEvent:event];
	}
	[NSApp updateWindows];
      }
    }

    // As part of cleanup, reset the audio hardware to the state we found it in
    OSStatus resetAudioResult = resetAudio();
    if (resetAudioResult)
    {
      return resetAudioResult;
    }
    IOReturn resetInputResult = resetInput();
    if (resetInputResult)
    {
      return resetInputResult;
    }

    [NSApp terminate:NSApp];
  }
  return 0;
}

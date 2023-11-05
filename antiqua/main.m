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
#include "osx_audio.h"
#include "osx_input.h"
#include "osx_lock.h"

static u8 shouldKeepRunning = 1;

static void processEvent(NSEvent *e)
{
  if (e.type == NSEventTypeKeyDown)
  {
    if (e.keyCode == kVK_Escape)
    {
      shouldKeepRunning = 0;
    }
    if (e.keyCode == kVK_ANSI_S && !gcInput.down.endedDown)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.down.halfTransitionCount++;
      gcInput.down.endedDown = 1;
    }
    if (e.keyCode == kVK_ANSI_D && !gcInput.right.endedDown)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.right.halfTransitionCount++;
      gcInput.right.endedDown = 1;
    }
    if (e.keyCode == kVK_ANSI_A && !gcInput.left.endedDown)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.left.halfTransitionCount++;
      gcInput.left.endedDown = 1;
    }
    if (e.keyCode == kVK_ANSI_W && !gcInput.up.endedDown)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.up.halfTransitionCount++;
      gcInput.up.endedDown = 1;
    }
  }
  else if (e.type == NSEventTypeKeyUp)
  {
    if (e.keyCode == kVK_ANSI_S)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.down.halfTransitionCount++;
      gcInput.down.endedDown = 0;
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.right.halfTransitionCount++;
      gcInput.right.endedDown = 0;
    }
    if (e.keyCode == kVK_ANSI_A)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.left.halfTransitionCount++;
      gcInput.left.endedDown = 0;
    }
    if (e.keyCode == kVK_ANSI_W)
    {
      waitIfBlocked(runThreadInput, runMutexInput, runConditionInput);
      gcInput.up.halfTransitionCount++;
      gcInput.up.endedDown = 0;
    }
  }
}

inline static u32 safeTruncateUInt64(u64 value)
{
  ASSERT(value <= 0xFFFFFFFF);
  u32 result = (u32) value;
  return result;
}

u8 debug_platformReadEntireFile(struct debug_ReadFileResult *outFile, const char *filename)
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

void debug_platformFreeFileMemory(struct debug_ReadFileResult *file)
{
  msync(file->contents, file->contentsSize, MS_SYNC);
  munmap(file->contents, file->contentsSize);
}

u8 debug_platformWriteEntireFile(const char *filename, u32 memorySize, void *memory)
{
  u8 result = 0;

  s32 fd = open(filename, O_WRONLY | O_EXLOCK | O_CREAT);
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

    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 600)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
    styleMask:NSTitledWindowMask | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
#pragma clang diagnostic pop
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
	NSEvent *event = [NSApp
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
	  nextEventMatchingMask:NSAnyEventMask
#pragma clang diagnostic pop
	  untilDate:[NSDate distantPast]
	  inMode:NSDefaultRunLoopMode
	  dequeue:YES];

	processEvent(event);

	[NSApp sendEvent:event];
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

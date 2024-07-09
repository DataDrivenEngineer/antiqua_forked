//
// Do not hot reload game dll if it is currently being compiled

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

#include "types.h"
#include "osx_audio.h"
#include "osx_input.h"
#include "osx_lock.h"

static b32 shouldKeepRunning = 1;

static void processButtonDown(GameButtonState *btn)
{
  if (!btn->endedDown)
  {
    waitIfInputBlocked(&thread);
    btn->halfTransitionCount++;
    btn->endedDown = 1;
  }
}

static void processButtonUp(GameButtonState *btn)
{
  waitIfInputBlocked(&thread);
  btn->halfTransitionCount++;
  btn->endedDown = 0;
}

static b32 processEvent(NSEvent *e)
{
  b32 handled = 0;

  NSWindow *window = [[NSApplication sharedApplication] mainWindow];
  NSView *view = [window contentView];
  NSPoint mouseLocScreen = [NSEvent mouseLocation];
  NSPoint mouseLocWindow = [window convertPointFromScreen:mouseLocScreen];
  NSPoint mouseLocView = [view convertPoint:mouseLocWindow fromView:nil];
  gcInput.mouseX = mouseLocView.x;
  gcInput.mouseY = mouseLocView.y;
  gcInput.mouseZ = 0;

  // NOTE(dima): we don't set handled = 1 for mouse events, because
  // we want to propagate the event downstream for OS to handle it
  if (e.type == NSEventTypeLeftMouseDown)
  {
    processButtonDown(&gcInput.mouseButtons[0]);
  }
  if (e.type == NSEventTypeLeftMouseUp)
  {
    processButtonUp(&gcInput.mouseButtons[0]);
  }
  if (e.type == NSEventTypeRightMouseDown)
  {
    processButtonDown(&gcInput.mouseButtons[1]);
  }
  if (e.type == NSEventTypeRightMouseUp)
  {
    processButtonUp(&gcInput.mouseButtons[1]);
  }
  if (e.type == NSEventTypeScrollWheel)
  {
      gcInput.scrollingDeltaX = e.scrollingDeltaX;
      gcInput.scrollingDeltaY = e.scrollingDeltaY;
  }

  if (e.type == NSEventTypeKeyDown)
  {
    if (e.keyCode == kVK_Escape)
    {
      handled = 1;
      shouldKeepRunning = 0;
    }
    if (e.keyCode == kVK_UpArrow)
    {
      handled = 1;
      processButtonDown(&gcInput.actionUp);
    }
    if (e.keyCode == kVK_ANSI_S)
    {
      handled = 1;
      processButtonDown(&gcInput.down);
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      handled = 1;
      processButtonDown(&gcInput.right);
    }
    if (e.keyCode == kVK_ANSI_A)
    {
      handled = 1;
      processButtonDown(&gcInput.left);
    }
    if (e.keyCode == kVK_ANSI_W)
    {
      handled = 1;
      processButtonDown(&gcInput.up);
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
    if (e.keyCode == kVK_Return)
    {
      handled = 1;
      // NOTE(dima): if Option/Alt key is pressed
      if (e.modifierFlags & NSEventModifierFlagOption)
      {
        state.toggleFullscreen = true;
      }
    }
    if (e.keyCode == kVK_ANSI_C)
    {
      handled = 1;
      // NOTE(dima): if Command key is pressed
      if (e.modifierFlags & NSEventModifierFlagCommand)
      {
          processButtonDown(&gcInput.debugCmdC);
      }
    }
  }
  else if (e.type == NSEventTypeKeyUp)
  {
    if (e.keyCode == kVK_UpArrow)
    {
      processButtonUp(&gcInput.actionUp);
      handled = 1;
    }
    if (e.keyCode == kVK_ANSI_S)
    {
      processButtonUp(&gcInput.down);
      handled = 1;
    }
    if (e.keyCode == kVK_ANSI_D)
    {
      handled = 1;
      processButtonUp(&gcInput.right);
    }
    if (e.keyCode == kVK_ANSI_A)
    {
      handled = 1;
      processButtonUp(&gcInput.left);
    }
    if (e.keyCode == kVK_ANSI_W)
    {
      handled = 1;
      processButtonUp(&gcInput.up);
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
  b32 result = 0;
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
          debug_platformFreeFileMemory(thread, outFile);
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
  b32 result = 0;

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
    NSPanel *window = [[NSPanel alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 700)
    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable | NSWindowStyleMaskNonactivatingPanel
    backing:NSBackingStoreBuffered defer:NO];
    // Uncomment the code below for looped live code editing
    [window setFloatingPanel:YES];
    // This will move app window to currently active space
//    [window setCollectionBehavior: NSWindowCollectionBehaviorCanJoinAllSpaces | NSWindowCollectionBehaviorFullScreenAuxiliary];
    // This will keep the app at the space where it was launched
    [window setCollectionBehavior: NSWindowCollectionBehaviorAuxiliary | NSWindowCollectionBehaviorFullScreenAuxiliary];
    [window setLevel: NSStatusWindowLevel];
#else
    NSWindow *window = [[NSWindow alloc] initWithContentRect:NSMakeRect(0, 0, 1000, 700)
    styleMask:NSWindowStyleMaskTitled | NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskResizable
    backing:NSBackingStoreBuffered defer:NO];
#endif
    [window setAcceptsMouseMovedEvents:YES];

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

        if (state.toggleFullscreen)
        {
          [window toggleFullScreen:window];
          state.toggleFullscreen = false;
        }

        NSEvent *event = [NSApp
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wdeprecated-declarations"
          nextEventMatchingMask:NSAnyEventMask
#pragma clang diagnostic pop
          untilDate:[NSDate distantPast]
          inMode:NSDefaultRunLoopMode
          dequeue:YES];

        b32 handled = processEvent(event);

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

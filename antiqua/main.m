//

#import <Cocoa/Cocoa.h>
#import <Carbon/Carbon.h>

#import "CustomNSView.h"
#import "CustomWindowDelegate.h"
#import "CustomCALayer.h"

#include "types.h"
#include "osx_audio.h"
#include "osx_input.h"

static u8 shouldKeepRunning = 1;

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
    }
    if (e.keyCode == kVK_ANSI_D)
    {
    }
  }
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

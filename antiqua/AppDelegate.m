//

#import "AppDelegate.h"
#import "CustomNSView.h"

@interface AppDelegate ()


@end

@implementation AppDelegate

- (void)applicationDidFinishLaunching:(NSNotification *)aNotification {
	// Insert code here to initialize your application
}

- (NSApplicationTerminateReply)applicationShouldTerminate:(NSApplication *)sender {
	return NSTerminateNow;
}


- (void)applicationWillTerminate:(NSNotification *)aNotification {
	// Insert code here to tear down your application
	[(CustomNSView *)((NSApplication *)aNotification.object).mainWindow.contentView releaseDisplayLink];
}

- (BOOL)applicationSupportsSecureRestorableState:(NSApplication *)app {
	return YES;
}


@end

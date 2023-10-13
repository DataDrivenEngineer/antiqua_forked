#import "CustomWindowDelegate.h"
#import "CustomNSView.h"

@interface CustomWindowDelegate ()

@end

@implementation CustomWindowDelegate

- (void)windowWillMiniaturize:(NSNotification *)notification
{
  [(CustomNSView *) ((NSWindow *) notification.object).contentView stopDisplayLink];
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
  [(CustomNSView *) ((NSWindow *) notification.object).contentView resumeDisplayLink];
}

- (void)windowWillClose:(NSNotification *)notification
{
  [(CustomNSView *) ((NSWindow *) notification.object).contentView stopDisplayLink];
}

@end

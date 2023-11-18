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

// Uncomment this for looped live code editing
//- (void)windowDidBecomeKey:(NSNotification *)notification
//{
//  ((NSWindow *) notification.object).opaque = NO;
//  ((NSWindow *) notification.object).alphaValue = 1.f;
//}
//
//- (void)windowDidResignKey:(NSNotification *)notification
//{
//  ((NSWindow *) notification.object).opaque = YES;
//  ((NSWindow *) notification.object).alphaValue = 0.3f;
//}

@end

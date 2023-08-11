#import "CustomWindowDelegate.h"
#import "CustomNSView.h"

@interface CustomWindowDelegate ()

@end

@implementation CustomWindowDelegate

- (void)windowWillMiniaturize:(NSNotification *)notification
{
  stopDisplayLink();
}

- (void)windowDidDeminiaturize:(NSNotification *)notification
{
  resumeDisplayLink();
}

- (void)windowWillClose:(NSNotification *)notification
{
  stopDisplayLink();
}

@end

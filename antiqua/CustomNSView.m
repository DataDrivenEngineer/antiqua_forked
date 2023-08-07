//

#import <AppKit/NSScreen.h>
#import <AppKit/NSGraphicsContext.h>
#import <CoreVideo/CVDisplayLink.h>
#import <AppKit/NSApplication.h>
#import <QuartzCore/CALayer.h>
#import <mach/mach_time.h>
#import <math.h>

#import "CustomNSView.h"
#import "CustomCALayer.h"

static struct mach_timebase_info mti;
static CustomCALayer *layer;
static CVDisplayLinkRef displayLink;

static void initTimebaseInfo(void)
{
  kern_return_t result;
  if ((result = mach_timebase_info(&mti)) != KERN_SUCCESS) printf("Failed to initialize timebase info. Error code: %d", result);
}

static void logFrameTime(const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime)
{
  static uint64_t previousNowNs = 0;
  uint64_t currentNowNs = inNow->hostTime * mti.numer / mti.denom;
  uint64_t inNowDiff = currentNowNs - previousNowNs;
  previousNowNs = currentNowNs;
  // Divide by 1000000 to convert from ns to ms
  NSLog(@"inNow frame time: %f ms", (float)inNowDiff / 1000000);
}

@implementation CustomNSView

static CVReturn renderCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
  logFrameTime(inNow, inOutputTime);
  CVReturn error = [(__bridge CustomNSView *) displayLinkContext displayFrame:inOutputTime];
  return error;
}

- (CVReturn)displayFrame:(const CVTimeStamp *)inOutputTime {
  dispatch_sync(dispatch_get_main_queue(), ^{
    [self setNeedsDisplay:YES];
  });
  return kCVReturnSuccess;
}

- (void)resumeDisplayLink {
  if (!CVDisplayLinkIsRunning(displayLink)) {
    CVDisplayLinkStart(displayLink);
  }
}

- (void)stopDisplayLink {
  if (CVDisplayLinkIsRunning(displayLink)) {
    CVDisplayLinkStop(displayLink);
  }
}

- (void) dealloc {
  [self stopDisplayLink];
  CVDisplayLinkRelease(displayLink);
}

- (void)releaseDisplayLink {
  [self stopDisplayLink];
}

- (id) initWithCoder:(NSCoder *)coder {
  self = [super initWithCoder:coder];
  
  layer = [[CustomCALayer alloc] init];
//  layer.bounds = self.bounds;
  NSLog(@"layerw x h: %f x %f", layer.bounds.size.width, layer.bounds.size.height);
  self.wantsLayer = YES;
  self.layer = layer;
  self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
  
  CGDirectDisplayID displayID = CGMainDisplayID();
  CVReturn error = kCVReturnSuccess;
  error = CVDisplayLinkCreateWithCGDisplay(displayID, &displayLink);
  if (error)
  {
    NSLog(@"DisplayLink created with error:%d", error);
    displayLink = NULL;
  }
  CVDisplayLinkSetOutputCallback(displayLink, renderCallback, (__bridge void *)self);
  CVDisplayLinkStart(displayLink);
  
  initTimebaseInfo();
  
  return self;
}

- (BOOL) wantsUpdateLayer
{
  return YES;
}

- (void)updateLayer {
  NSLog(@"updateLayer - never called!");
}

@end

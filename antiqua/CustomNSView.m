//

#import <AppKit/NSScreen.h>
#import <AppKit/NSGraphicsContext.h>
#import <CoreVideo/CVDisplayLink.h>
#import <AppKit/NSApplication.h>
#import <mach/mach_time.h>
#import <math.h>

#import "CustomNSView.h"

typedef struct 
{
  uint16_t width;
  uint16_t height;
  uint8_t *memory;
} Framebuffer;

static Framebuffer framebuffer;
static struct mach_timebase_info mti;

static void initTimebaseInfo(void)
{
  kern_return_t result;
  if ((result = mach_timebase_info(&mti)) != KERN_SUCCESS) printf("Failed to initialize timebase info. Error code: %d", result);
}

float getVectorLength(float x, float y) {
  float length = sqrtf(pow(x, 2) + pow(y, 2));
  return length;
}

static void logFrameTime(const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime)
{
  static uint64_t previousNowNs = 0;
  uint64_t currentNowNs = inNow->hostTime * mti.numer / mti.denom;
  uint64_t inNowDiff = currentNowNs - previousNowNs;
  previousNowNs = currentNowNs;
  // Divide by 1000000 to convert from ns to ms
  NSLog(@"inNow frame time: %f ms", (float)inNowDiff / 1000000);

//  static uint64_t previousOutputTimeNs = 0;
//  uint64_t currentOutputTimeNs = inOutputTime->hostTime * mti.numer / mti.denom;
//  uint64_t inOutputTimeDiff = currentOutputTimeNs - previousOutputTimeNs;
//  previousOutputTimeNs = currentOutputTimeNs;
//  // Divide by 1000000 to convert from ns to ms
//  NSLog(@"inOutputTime frame time: %f ms", (float)inOutputTimeDiff / 1000000);
}

@implementation CustomNSView
{
  float scaleFactor;
  CGSize size;
  size_t circleRadius;
  CVDisplayLinkRef displayLink;
}

static CVReturn renderCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
  logFrameTime(inNow, inOutputTime);
  CVReturn error = [(__bridge CustomNSView*) displayLinkContext displayFrame:inOutputTime];
  return error;
}

static const void * getBytePointerCallback(void *info)
{
  return (const void *)info;
}

static void releaseBytePointerCallback(void *info, const void *pointer)
{
  free(info);
}

- (CVReturn)displayFrame:(const CVTimeStamp *)inOutputTime {
  dispatch_sync(dispatch_get_main_queue(), ^{
    [self setNeedsDisplay:YES];
//    [self display];
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
  free(framebuffer.memory);
  [self stopDisplayLink];
  CVDisplayLinkRelease(displayLink);
}

- (void)releaseDisplayLink {
  [self stopDisplayLink];
  [[NSApplication sharedApplication] replyToApplicationShouldTerminate:YES];
}

- (id) initWithCoder:(NSCoder *)coder {
  self = [super initWithCoder:coder];
  
  NSRect screenFrame = [[NSScreen mainScreen] frame];
  NSLog(@"screen w x h: %f x %f", screenFrame.size.width, screenFrame.size.height);

  scaleFactor = [[NSScreen mainScreen] backingScaleFactor];


  NSLog(@"%@",[[NSBundle mainBundle] resourcePath]);
  
  circleRadius = 500.0f;
  size = [[NSScreen mainScreen] convertRectToBacking:[[NSScreen mainScreen] frame]].size;
  
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

- (void)drawRect:(NSRect)dirtyRect {
  printf("stdout?");
//  size.width = dirtyRect.size.width * 2;
//  size.height = dirtyRect.size.height * 2;
  size_t resScale = 10;
  size.width = 16 * resScale;
  size.height = 10 * resScale;
  circleRadius = size.width < size.height ? size.width / 2 : size.height / 2;
  NSLog(@"w: %f, h: %f", size.width, size.height);
  size_t bitmapSize = sizeof(uint8_t) * size.width * 4 * size.height;
  
  // Multiplying by scaleFactor to have 4x more pixels for Retina screen
//  framebuffer.width = screenFrame.size.width * scaleFactor;
//  framebuffer.height = screenFrame.size.height * scaleFactor;
  framebuffer.memory = malloc(bitmapSize);

//  uint8_t* end = framebuffer.memory + bitmapSize;
//  for (uint8_t* p = framebuffer.memory; p != end; ++p) {
//    if ((p - framebuffer.memory + 1) % 4 == 0) *p = 255;
//    // Start of new pixel
//    else if ((p - framebuffer.memory) % 4 == 0) {
//      size_t currentPointX = ((p - framebuffer.memory) / 4) % (size_t) size.width;
//      size_t currentPointY = ((p - framebuffer.memory) / 4) / size.width;
//      float vectorLength = getVectorLength((float) (currentPointX - size.width / 2), (float) (currentPointY - size.height / 2));
//      if (vectorLength <= circleRadius) {
//	*p++ = 0;
//	*p++ = 255;
//	*p = 0;
//      } else *p = 0;
//    } else *p = 0;
//  }

  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
//  CGContextRef context = CGBitmapContextCreate(framebuffer.memory, size.width, size.height, 8, size.width * 4, colorSpace, kCGImageAlphaPremultipliedLast);
  struct CGDataProviderDirectCallbacks callbacks;
  callbacks.version = 0;
  callbacks.getBytePointer = getBytePointerCallback;
  callbacks.releaseBytePointer = releaseBytePointerCallback;
  CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, bitmapSize, &callbacks);

//  CGImageRef image = CGBitmapContextCreateImage(context);
  CGImageRef image = CGImageCreate(size.width, size.height, 8, 32, size.width * 4, colorSpace, kCGImageAlphaPremultipliedLast, dataProvider, NULL, true, kCGRenderingIntentDefault);

  CGContextRef currentContext = NSGraphicsContext.currentContext.CGContext;
  CGContextDrawImage(currentContext, dirtyRect, image);

  CGImageRelease(image);
//  CGContextRelease(context);
}

@end

#import "CustomCALayer.h"
#import "CustomNSView.h"
#import "types.h"

static CGImageRef image;
static CGColorSpaceRef colorSpace;
static struct CGDataProviderDirectCallbacks callbacks;

static const void * getBytePointerCallback(void *info)
{
  return (const void *)info;
}

static void releaseBytePointerCallback(void *info, const void *pointer) {}

@implementation CustomCALayer

- (instancetype)init
{
  self = [super init];
  
  colorSpace = CGColorSpaceCreateDeviceRGB();
  callbacks.version = 0;
  callbacks.getBytePointer = getBytePointerCallback;
  callbacks.releaseBytePointer = releaseBytePointerCallback;

  return self;
}

- (void)displayLayer:(CALayer *)theLayer
{
  NSLog(@"displayLayer - never called!");
}

- (void)drawInContext:(CGContextRef)ctx
{
  @autoreleasepool
  {
    NSScreen *screen = [NSScreen mainScreen];
    NSDictionary *description = [screen deviceDescription];
    // NOTE(dima): maximum resolution of current main display
    CGSize displayPixelSize = NSSizeToCGSize([[description objectForKey:NSDeviceSize] sizeValue]);

    s32 offsetX = 0;
    s32 offsetY = 0;
    CGRect framebufferRect;

    NSWindow *window = [[NSApplication sharedApplication] mainWindow];
    if (CGSizeEqualToSize(window.frame.size, displayPixelSize))
    {
      framebufferRect = CGContextGetClipBoundingBox(ctx);
    }
    else
    {
      // NOTE(dima): Do not scale the image when not in full-screen
      framebufferRect = CGRectMake(0, 0, framebuffer.width, framebuffer.height);
      offsetX = 10;
      // NOTE(dima): y is negative, because we are flipping the Y axis for the image
      offsetY = -10;
    }

    // Clear the screen
    CGContextClearRect(ctx, framebufferRect);

    CGRect framebufferRectWithOffset = CGRectMake(offsetX, offsetY, framebufferRect.size.width, framebufferRect.size.height);
    
    CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, framebuffer.sizeBytes, &callbacks);

    image = CGImageCreate(framebuffer.width, framebuffer.height, 8, 32, framebuffer.width * 4, colorSpace, 
    kCGBitmapByteOrder32Little | kCGImageAlphaNoneSkipFirst,
    dataProvider, NULL, false, kCGRenderingIntentDefault);

    // NOTE(dima): Flip the image upside down, because by default Quartz uses bottom left corner as the origin,
    // while we want the origin in the top left one
    CGContextTranslateCTM(ctx, 0, framebufferRect.size.height);
    CGContextScaleCTM(ctx, 1.0, -1.0);

    CGContextDrawImage(ctx, framebufferRectWithOffset, image);

    CGDataProviderRelease(dataProvider);
    CGImageRelease(image);
  }
}

@end

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
//    CGRect dirtyRect = CGContextGetClipBoundingBox(ctx);
    // Do not scale the image when window is resized
    CGRect framebufferRect = CGRectMake(0, 0, framebuffer.width, framebuffer.height);
    
    CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, framebuffer.sizeBytes, &callbacks);

    image = CGImageCreate(framebuffer.width, framebuffer.height, 8, 32, framebuffer.width * 4, colorSpace, 
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wenum-conversion"
    kCGImageAlphaNoneSkipLast,
#pragma clang diagnostic pop
    dataProvider, NULL, false, kCGRenderingIntentDefault);

    CGContextDrawImage(ctx, framebufferRect, image);

    CGDataProviderRelease(dataProvider);
    CGImageRelease(image);
  }
}

@end

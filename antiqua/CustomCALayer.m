#import "CustomCALayer.h"
#import "antiqua.h"
#import "types.h"

static struct GameOffscreenBuffer framebuffer;
static CGImageRef image;
static CGColorSpaceRef colorSpace;
static struct CGDataProviderDirectCallbacks callbacks;

static u64 xOff = 0;
static u64 yOff = 0;

void incXOff(s32 val)
{
  xOff += val;
}

void incYOff(s32 val)
{
  yOff += val;
}

static const void * getBytePointerCallback(void *info)
{
  return (const void *)info;
}

static void releaseBytePointerCallback(void *info, const void *pointer)
{
  free(info);
}


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

- (void) dealloc
{
  CGColorSpaceRelease(colorSpace);
}

- (void)displayLayer:(CALayer *)theLayer
{
  NSLog(@"displayLayer - never called!");
}

- (void)drawInContext:(CGContextRef)ctx
{
  @autoreleasepool
  {
    CGRect dirtyRect = CGContextGetClipBoundingBox(ctx);
    framebuffer.width = 1024;
    framebuffer.height = 640;
    size_t bitmapSize = sizeof(uint8_t) * framebuffer.width * 4 * framebuffer.height;
    
    framebuffer.memory = malloc(bitmapSize);
    
    updateGameAndRender(&framebuffer, xOff, yOff);

    CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, bitmapSize, &callbacks);

    image = CGImageCreate(framebuffer.width, framebuffer.height, 8, 32, framebuffer.width * 4, colorSpace, kCGImageAlphaNoneSkipLast, dataProvider, NULL, false, kCGRenderingIntentDefault);

    CGContextDrawImage(ctx, dirtyRect, image);

    CGDataProviderRelease(dataProvider);
    CGImageRelease(image);
  }
}

@end

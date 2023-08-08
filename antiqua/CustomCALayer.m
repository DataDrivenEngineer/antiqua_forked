#import "CustomCALayer.h"

typedef struct 
{
  double_t width;
  double_t height;
  uint8_t *memory;
} Framebuffer;

static Framebuffer framebuffer;
static CGImageRef image;
static CGColorSpaceRef colorSpace;
static struct CGDataProviderDirectCallbacks callbacks;

static const void * getBytePointerCallback(void *info)
{
  return (const void *)info;
}

static void releaseBytePointerCallback(void *info, const void *pointer)
{
  free(info);
}

static void renderGradient(int xOffset, int yOffset)
{
    int pitch = framebuffer.width * 4;
    uint8_t *row = framebuffer.memory;
    for (int y = 0; y < framebuffer.height; y++)
    {
      uint8_t *pixel = row;
      for (int x = 0; x < framebuffer.width; x++)
      {
	*pixel++ = 0;
	*pixel++ = y + yOffset;
	*pixel++ = x + xOffset;
	pixel++;
      }
      row += pitch;
    }
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
    
    static uint64_t xOff = 0;
    static uint64_t yOff = 0;
    renderGradient(xOff++, yOff++);

    CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, bitmapSize, &callbacks);

    image = CGImageCreate(framebuffer.width, framebuffer.height, 8, 32, framebuffer.width * 4, colorSpace, kCGImageAlphaNoneSkipLast, dataProvider, NULL, false, kCGRenderingIntentDefault);

    CGContextDrawImage(ctx, dirtyRect, image);

    CGDataProviderRelease(dataProvider);
    CGImageRelease(image);
  }
}

@end

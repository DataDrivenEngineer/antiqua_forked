#import "CustomCALayer.h"

typedef struct 
{
  uint16_t width;
  uint16_t height;
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
    CGFloat width = dirtyRect.size.width;
    CGFloat height = dirtyRect.size.height;
    size_t bitmapSize = sizeof(uint8_t) * width * 4 * height;
    
    framebuffer.memory = malloc(bitmapSize);
    
    int pitch = width * 4;
    uint8_t *row = framebuffer.memory;
    for (int y = 0; y < height; y++)
    {
      uint8_t *pixel = row;
      for (int x = 0; x < width; x++)
      {
	*pixel++ = 0;
	*pixel++ = y;
	*pixel++ = x;
	*pixel++ = 0;
      }
      row += pitch;
    }

    CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, bitmapSize, &callbacks);

    image = CGImageCreate(width, height, 8, 32, width * 4, colorSpace, kCGImageAlphaNoneSkipLast, dataProvider, NULL, false, kCGRenderingIntentDefault);

    CGContextDrawImage(ctx, dirtyRect, image);

    CGDataProviderRelease(dataProvider);
    CGImageRelease(image);
  }
}

@end

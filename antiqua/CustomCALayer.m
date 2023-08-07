#import <AppKit/NSScreen.h>

#import "CustomCALayer.h"

typedef struct 
{
  uint16_t width;
  uint16_t height;
  uint8_t *memory;
} Framebuffer;

static Framebuffer framebuffer;

float getVectorLength(float x, float y) {
  float length = sqrtf(pow(x, 2) + pow(y, 2));
  return length;
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

- (void)displayLayer:(CALayer *)theLayer
{
  NSLog(@"displayLayer!");
}

- (void)drawInContext:(CGContextRef)ctx
{
  NSLog(@"drawInContext!");
  
  CGSize size = CGContextGetClipBoundingBox(ctx).size;
//  CGSize size;
//  size.width = 640;
//  size.height = 400;
  size_t circleRadius;
  circleRadius = size.width < size.height ? size.width / 2 : size.height / 2;
  NSLog(@"w: %f, h: %f", size.width, size.height);
  size_t bitmapSize = sizeof(uint8_t) * size.width * 4 * size.height;
  
  framebuffer.memory = malloc(bitmapSize);

  uint8_t* end = framebuffer.memory + bitmapSize;
  for (uint8_t* p = framebuffer.memory; p != end; ++p) {
    // Start of new pixel
    if ((p - framebuffer.memory) % 4 == 0) {
      size_t currentPointX = ((p - framebuffer.memory) / 4) % (size_t) size.width;
      size_t currentPointY = ((p - framebuffer.memory) / 4) / size.width;
      float vectorLength = getVectorLength((float) (currentPointX - size.width / 2), (float) (currentPointY - size.height / 2));
      if (vectorLength <= circleRadius) {
	*p++ = 0;
	*p++ = 255;
	*p = 0;
      } else *p = 0;
    } else *p = 0;
  }

  CGColorSpaceRef colorSpace = CGColorSpaceCreateDeviceRGB();
  struct CGDataProviderDirectCallbacks callbacks;
  callbacks.version = 0;
  callbacks.getBytePointer = getBytePointerCallback;
  callbacks.releaseBytePointer = releaseBytePointerCallback;
  CGDataProviderRef dataProvider = CGDataProviderCreateDirect(framebuffer.memory, bitmapSize, &callbacks);

  CGImageRef image = CGImageCreate(size.width, size.height, 8, 32, size.width * 4, colorSpace, kCGImageAlphaNoneSkipLast, dataProvider, NULL, false, kCGRenderingIntentDefault);

  CGContextDrawImage(ctx, CGContextGetClipBoundingBox(ctx), image);
}

@end

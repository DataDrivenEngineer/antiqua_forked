//

#import <AppKit/NSScreen.h>
#import <AppKit/NSGraphicsContext.h>
#import <CoreVideo/CVDisplayLink.h>
#import <AppKit/NSApplication.h>
#import <QuartzCore/CALayer.h>
#import <math.h>
#import <sys/types.h>
#import <sys/mman.h>
#import <sys/stat.h>

#import "osx_audio.h"
#import "osx_time.h"
#import "osx_input.h"
#import "types.h"
#import "CustomNSView.h"
#import "CustomCALayer.h"
#import "antiqua.h"
#import "osx_dynamic_loader.h"

struct GameOffscreenBuffer framebuffer;
struct GameMemory gameMemory = {0};

static CustomCALayer *layer;
static CVDisplayLinkRef displayLink;
static u8 shouldStopDL = 0;

static u8 skipCurrentFrame = 1;

static void logFrameTime(const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime)
{
  static u64 previousNowNs = 0;
  u64 currentNowNs = TIMESTAMP_TO_NS(inNow->hostTime);
  u64 inNowDiff = currentNowNs - previousNowNs;
  previousNowNs = currentNowNs;
  // Divide by 1000000 to convert from ns to ms
  NSLog(@"inNow frame time: %f ms", (r32)inNowDiff / 1000000);

//  u64 processingWindowNs = (inOutputTime->hostTime - inNow->hostTime) * mti.numer / mti.denom;
//  NSLog(@"processingWindow: %f ms", (r32)processingWindowNs / 1000000);
}

static CVReturn renderCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
  skipCurrentFrame = !skipCurrentFrame;
  if (!skipCurrentFrame)
  {
//    logFrameTime(inNow, inOutputTime);
    CVReturn error = [(__bridge CustomNSView *) displayLinkContext displayFrame:inOutputTime];
    return error;
  }
  return kCVReturnSuccess;
}

@implementation CustomNSView

-(void) resumeDisplayLink
{
  shouldStopDL = 0;
  if (!CVDisplayLinkIsRunning(displayLink)) CVDisplayLinkStart(displayLink);

}

-(void) doStop
{
  if (CVDisplayLinkIsRunning(displayLink)) {
    CVDisplayLinkStop(displayLink);
  }
}

-(void) stopDisplayLink
{
  shouldStopDL = 1;
}

- (CVReturn)displayFrame:(const CVTimeStamp *)inOutputTime {
  struct stat st;
  if (stat(GAME_CODE_LIB_NAME, &st) != -1)
  {
    IF_IS_GAME_CODE_MODIFIED(st.st_mtimespec, gameCode.lastModified)
    {
      waitIfInputBlocked();
      lockInputThread();
      waitIfAudioBlocked();
      lockAudioThread();
      unloadGameCode();
      loadGameCode(st.st_mtimespec);
      unlockAudioThread();
      unlockInputThread();
    }
  }

  if (shouldStopDL)
  {
    [self doStop];
  }
  else
  {
    gameCode.updateGameAndRender(&gcInput, &soundState, &gameMemory, &framebuffer);

    dispatch_sync(dispatch_get_main_queue(), ^{
      [self setNeedsDisplay:YES];
    });
  }
  return kCVReturnSuccess;
}


- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];
  
  layer = [[CustomCALayer alloc] init];
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

  // init game memory
  gameMemory.permanentStorageSize = MB(64);
  gameMemory.transientStorageSize = GB(2);
  u64 totalStorageSize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
  // allocate physical memory
  // equivalent of VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE) - except that memory is going to be committed on as-needed basis when we write to it
#if ANTIQUA_INTERNAL
  u64 baseAddress = GB(10);
  gameMemory.permanentStorage = mmap((void *) baseAddress, totalStorageSize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
  msync((void *) baseAddress, gameMemory.permanentStorageSize, MS_SYNC | MS_INVALIDATE);
#else
  gameMemory.permanentStorage = mmap(0, totalStorageSize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  msync(gameMemory.permanentStorage, gameMemory.permanentStorageSize, MS_SYNC | MS_INVALIDATE);
#endif
  if (gameMemory.permanentStorage == MAP_FAILED)
  {
    fprintf(stderr, "Failed to allocate permanentStorage - error: %d\n", errno);
    return 0;
  }
  gameMemory.transientStorage = (u8 *) gameMemory.permanentStorage + gameMemory.permanentStorageSize;

  gameMemory.resetInputStateButtons = resetInputStateButtons;

  gameMemory.lockAudioThread = lockAudioThread;
  gameMemory.unlockAudioThread = unlockAudioThread;
  gameMemory.waitIfAudioBlocked = waitIfAudioBlocked;
  gameMemory.lockInputThread = lockInputThread;
  gameMemory.unlockInputThread = unlockInputThread;
  gameMemory.waitIfInputBlocked = waitIfInputBlocked;

  gameMemory.debug_platformFreeFileMemory = debug_platformFreeFileMemory;
  gameMemory.debug_platformReadEntireFile = debug_platformReadEntireFile;
  gameMemory.debug_platformWriteEntireFile = debug_platformWriteEntireFile;

  // init game specific state
  framebuffer.width = 1024;
  framebuffer.height = 640;
  framebuffer.sizeBytes = sizeof(u8) * framebuffer.width * 4 * framebuffer.height;
  framebuffer.memory = malloc(framebuffer.sizeBytes);
  
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

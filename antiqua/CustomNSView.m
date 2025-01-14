//

#import <AppKit/NSScreen.h>
#import <AppKit/NSGraphicsContext.h>
#import <CoreVideo/CVDisplayLink.h>
#import <AppKit/NSApplication.h>

#import <Metal/MTLDevice.h>
#import <QuartzCore/CAMetalLayer.h>

#import <math.h>
#import <sys/types.h>
#import <sys/mman.h>
#import <sys/stat.h>
#import <unistd.h>
#import <sys/types.h>
#import <fcntl.h>
#import <dlfcn.h>

#import "osx_audio.h"
#import "osx_time.h"
#import "osx_input.h"
#import "types.h"
#import "CustomNSView.h"
#import "osx_dynamic_loader.h"
#import "osx_renderer.h"

#define LOOP_EDIT_FILENAME "tmp/loop_edit.atqi"

GameMemory gameMemory;

static CVDisplayLinkRef displayLink;
static b32 shouldStopDL = 0;

static r32 drawableWidthWithoutScaleFactor;
static r32 drawableHeightWithoutScaleFactor;

ThreadContext thread = {0};
State state = {0};

void beginRecordingInput(State *state, s32 inputRecordingIndex)
{
  state->inputRecordingIndex = inputRecordingIndex;
  const char *filename = LOOP_EDIT_FILENAME;
  state->recordingHandle = open(filename, O_WRONLY | O_EXLOCK | O_CREAT, 0777);

  u32 bytesToWrite = (u32) state->totalMemorySize;
  ASSERT(bytesToWrite < 0xFFFFFFFF);
  write(state->recordingHandle, state->gameMemoryBlock, bytesToWrite);
}

void endRecordingInput(State *state)
{
  if (close(state->recordingHandle) == -1)
  {
    fprintf(stderr, "Failed to close recording handle, error: %d\n", errno);
  }
  state->inputRecordingIndex = 0;
}

void beginInputPlayBack(State *state, s32 inputPlayingIndex)
{
  state->inputPlayingIndex = inputPlayingIndex;
  const char *filename = LOOP_EDIT_FILENAME;
  state->playBackHandle = open(filename, O_RDONLY | O_SHLOCK);

  u32 bytesToRead = (u32) state->totalMemorySize;
  ASSERT(bytesToRead < 0xFFFFFFFF);
  read(state->playBackHandle, state->gameMemoryBlock, state->totalMemorySize);
}

void endInputPlayBack(State *state)
{
  close(state->playBackHandle);
  state->inputPlayingIndex = 0;
}

static void recordInput(State *state, GameControllerInput *gcInput)
{
  write(state->recordingHandle, gcInput, sizeof(*gcInput));
}

static void playBackInput(State *state, GameControllerInput *gcInput)
{
  s32 bytesRead = read(state->playBackHandle, gcInput, sizeof(*gcInput));
  if (bytesRead == 0)
  {
    s32 playingIndex = state->inputPlayingIndex;
    endInputPlayBack(state);
    beginInputPlayBack(state, playingIndex);
    bytesRead = read(state->playBackHandle, gcInput, sizeof(*gcInput));
  }
  else if (bytesRead == -1)
  {
    fprintf(stderr, "Failed to read input to play back, error: %s\n", dlerror());
  }
}

static CVReturn renderCallback(CVDisplayLinkRef displayLink, const CVTimeStamp *inNow, const CVTimeStamp *inOutputTime, CVOptionFlags flagsIn, CVOptionFlags *flagsOut, void *displayLinkContext)
{
    static u64 previousNowNs = 0;
    u64 currentNowNs = TIMESTAMP_TO_NS(inNow->hostTime);
    u64 inNowDiff = currentNowNs - previousNowNs;
    previousNowNs = currentNowNs;
    // Divide by 1000000 to convert from ns to ms
    //    fprintf(stderr, "inNow frame time: %f ms\n", (r32)inNowDiff / 1000000);

    r32 deltaTimeSec = (r32) inNowDiff / 1000000000;
    CVReturn error = [(__bridge CustomNSView *) displayLinkContext displayFrame:deltaTimeSec];
    return error;
}

@implementation CustomNSView

// NOTE(dima): this ensures that view's top left corner is at window's top left corner
// By default, view's top left corner is at bottom left corner of a window
- (BOOL) isFlipped
{
  return YES;
}

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

- (CVReturn)displayFrame:(r32)deltaTimeSec {
#if !ANTIQUA_INTERNAL
  [NSCursor hide];
#endif

#if !XCODE_BUILD
  s32 fd = open("lock.tmp", O_RDONLY);
  // Do not hot reload game dll if it is currently being compiled
  if (fd == -1 && errno == ENOENT)
  {
    struct stat st;
    if (stat(GAME_CODE_LIB_NAME, &st) != -1)
    {
      IF_IS_GAME_CODE_MODIFIED(st.st_mtimespec, gameCode.lastModified)
      {
        waitIfInputBlocked(&thread);
        lockInputThread(&thread);
        waitIfAudioBlocked(&thread);
        lockAudioThread(&thread);
        unloadGameCode();
        loadGameCode(st.st_mtimespec);
        unlockAudioThread(&thread);
        unlockInputThread(&thread);
      }
    }
    else
    {
        fprintf(stderr, "Failed to stat library with game code, error: %s\n", strerror(errno));
    }
  }
#endif

  if (shouldStopDL)
  {
    [self doStop];
  }
  else
  {
    if (state.inputRecordingIndex)
    {
      recordInput(&state, &gcInput);
    }
    if (state.inputPlayingIndex)
    {
      playBackInput(&state, &gcInput);
    }

#if !XCODE_BUILD
    if (gameCode.updateGameAndRender)
    {
      gameCode.updateGameAndRender(&thread, deltaTimeSec, &gcInput, &soundState, &gameMemory, drawableWidthWithoutScaleFactor, drawableHeightWithoutScaleFactor);
    }
#else
    updateGameAndRender(&thread, deltaTimeSec, &gcInput, &soundState, &gameMemory, drawableWidthWithoutScaleFactor, drawableHeightWithoutScaleFactor);
#endif

    dispatch_sync(dispatch_get_main_queue(), ^{
      [self setNeedsDisplay:YES];
    });
  }
  return kCVReturnSuccess;
}

-(void)frameDidChangeCallback:(NSNotification *)notification
{
    CAMetalLayer *layer = (CAMetalLayer *) self.layer;
    drawableWidthWithoutScaleFactor = self.frame.size.width;
    drawableHeightWithoutScaleFactor = self.frame.size.height;
    CGFloat scaleFactor = [[NSApplication sharedApplication] mainWindow].backingScaleFactor;
    layer.drawableSize = CGSizeMake(self.frame.size.width * scaleFactor,
                                    self.frame.size.height * scaleFactor);
}

- (instancetype)initWithFrame:(NSRect)frameRect {
  self = [super initWithFrame:frameRect];

  self.postsBoundsChangedNotifications = true;
  
  CAMetalLayer *layer = [CAMetalLayer layer];
  layer.device = MTLCreateSystemDefaultDevice();
  self.layer = layer;
  self.layerContentsRedrawPolicy = NSViewLayerContentsRedrawOnSetNeedsDisplay;
  initRenderer(layer);

  [NSNotificationCenter.defaultCenter addObserver:self
                                      selector:@selector(frameDidChangeCallback:)
                                      name:NSViewFrameDidChangeNotification
                                      object:0];

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
  gameMemory.permanentStorageSize = GB(1);
  gameMemory.transientStorageSize = GB(1);
  state.totalMemorySize = gameMemory.permanentStorageSize + gameMemory.transientStorageSize;
  // allocate physical memory
  // equivalent of VirtualAlloc(addr, size, MEM_COMMIT, PAGE_READWRITE) - except that memory is going to be committed on as-needed basis when we write to it
#if ANTIQUA_INTERNAL
  u64 baseAddress = GB(1024);
  state.gameMemoryBlock = mmap((void *) baseAddress, state.totalMemorySize, PROT_READ | PROT_WRITE, MAP_FIXED | MAP_PRIVATE | MAP_ANON, -1, 0);
  gameMemory.permanentStorage = state.gameMemoryBlock;
  msync((void *) baseAddress, gameMemory.permanentStorageSize, MS_SYNC | MS_INVALIDATE);
#else
  gameMemory.permanentStorage = mmap(0, state.totalMemorySize, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANON, -1, 0);
  msync(gameMemory.permanentStorage, gameMemory.permanentStorageSize, MS_SYNC | MS_INVALIDATE);
#endif
  if (gameMemory.permanentStorage == MAP_FAILED)
  {
    fprintf(stderr, "Failed to allocate permanentStorage - error: %d\n", errno);
    return 0;
  }
  state.gameMemoryBlock = gameMemory.permanentStorage;
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

  gameMemory.renderOnGpu = renderOnGpu;
  
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

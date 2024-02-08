//

#ifndef CustomView_h
#define CustomView_h

#import <AppKit/NSView.h>
#import <CoreVideo/CVBase.h>
#import <CoreVideo/CVReturn.h>

#import "antiqua_platform.h"

typedef struct
{
  u64 totalMemorySize;
  void *gameMemoryBlock;

  s32 recordingHandle;
  s32 inputRecordingIndex;
  s32 playBackHandle;
  s32 inputPlayingIndex;

  b32 toggleFullscreen;
} State;

extern ThreadContext thread;
extern State state;

void beginRecordingInput(State *state, s32 inputRecordingIndex);
void endRecordingInput(State *state);
void beginInputPlayBack(State *state, s32 inputPlayingIndex);
void endInputPlayBack(State *state);

@interface CustomNSView : NSView

- (BOOL) isFlipped;

- (void) resumeDisplayLink;
- (void) stopDisplayLink;
- (CVReturn)displayFrame:(r32)deltaTimeSec;
- (void)updateLayer;

@end

#endif /* CustomView_h */

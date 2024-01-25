//

#ifndef CustomView_h
#define CustomView_h

#import <AppKit/NSView.h>
#import <CoreVideo/CVBase.h>
#import <CoreVideo/CVReturn.h>
#import "antiqua.h"

struct State
{
  u64 totalMemorySize;
  void *gameMemoryBlock;

  s32 recordingHandle;
  s32 inputRecordingIndex;
  s32 playBackHandle;
  s32 inputPlayingIndex;

  b32 toggleFullscreen;
};

extern struct ThreadContext thread;
extern struct State state;

void beginRecordingInput(struct State *state, s32 inputRecordingIndex);
void endRecordingInput(struct State *state);
void beginInputPlayBack(struct State *state, s32 inputPlayingIndex);
void endInputPlayBack(struct State *state);

@interface CustomNSView : NSView

- (BOOL) isFlipped;

- (void) resumeDisplayLink;
- (void) stopDisplayLink;
- (CVReturn)displayFrame:(r32)deltaTimeSec;
- (void)updateLayer;

@end

#endif /* CustomView_h */

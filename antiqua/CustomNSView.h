//

#ifndef CustomView_h
#define CustomView_h

#import <AppKit/NSView.h>
#import <CoreVideo/CVBase.h>
#import <CoreVideo/CVReturn.h>

@interface CustomNSView : NSView

- (CVReturn)displayFrame:(const CVTimeStamp *)inOutputTime;
- (void)resumeDisplayLink;
- (void)stopDisplayLink;
- (void)releaseDisplayLink;
- (void)updateLayer;

@end

#endif /* CustomView_h */

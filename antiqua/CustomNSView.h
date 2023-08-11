//

#ifndef CustomView_h
#define CustomView_h

#import <AppKit/NSView.h>
#import <CoreVideo/CVBase.h>
#import <CoreVideo/CVReturn.h>

@interface CustomNSView : NSView

void resumeDisplayLink(void);
void stopDisplayLink(void);

- (CVReturn)displayFrame:(const CVTimeStamp *)inOutputTime;
- (void)updateLayer;

@end

#endif /* CustomView_h */

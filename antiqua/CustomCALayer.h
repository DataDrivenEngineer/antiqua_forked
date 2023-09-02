#ifndef CustomCALayer_h
#define CustomCALayer_h

#import <QuartzCore/CALayer.h>

#import "types.h"

@interface CustomCALayer : CALayer

void incXOff(s32 val);
void incYOff(s32 val);

@end

#endif

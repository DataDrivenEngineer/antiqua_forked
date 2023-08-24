#ifndef _ANTIQUA_H_
#define _ANTIQUA_H_

#include "types.h"

struct GameOffscreenBuffer
{
  u32 width;
  u32 height;
  u8 *memory;
};

void updateGameAndRender(struct GameOffscreenBuffer *buf, u64 xOff, u64 yOff);

#endif

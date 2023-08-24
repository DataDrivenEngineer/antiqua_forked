#include "antiqua.h"

static void renderGradient(struct GameOffscreenBuffer *buf, int xOffset, int yOffset)
{
    int pitch = buf->width * 4;
    u8 *row = buf->memory;
    for (int y = 0; y < buf->height; y++)
    {
      u8 *pixel = row;
      for (int x = 0; x < buf->width; x++)
      {
	*pixel++ = 0;
	*pixel++ = y + yOffset;
	*pixel++ = x + xOffset;
	pixel++;
      }
      row += pitch;
    }
}

void updateGameAndRender(struct GameOffscreenBuffer *buf, u64 xOff, u64 yOff)
{
  renderGradient(buf, xOff, yOff);
}

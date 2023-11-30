#ifndef _OSX_DYNAMIC_LOADER_H_
#define _OSX_DYNAMIC_LOADER_H_

#include <time.h>

#include "types.h"
#include "antiqua.h"

#define GAME_CODE_LIB_NAME "libantiqua.dylib"
#define GAME_CODE_TMPLIB_NAME "libantiquatmp.dylib"

// NOTE(dima): check two 'struct timespec's for equality
#define IF_IS_GAME_CODE_MODIFIED(one, two) if ((one).tv_sec != (two).tv_sec || (one).tv_nsec != (two).tv_nsec)

struct AntiquaCode
{
  struct timespec lastModified;
  void *gameCodeDylib;
  UpdateGameAndRender *updateGameAndRender;
  FillSoundBuffer *fillSoundBuffer;
};

extern struct AntiquaCode gameCode;

b32 loadGameCode(struct timespec newLastModified);
void unloadGameCode(void);

#endif

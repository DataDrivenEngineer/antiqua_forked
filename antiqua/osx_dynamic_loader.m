#import <dlfcn.h>
#import <copyfile.h>

#import "osx_dynamic_loader.h"

struct AntiquaCode gameCode = {0};

u8 loadGameCode(struct timespec newLastModified)
{
  u8 result = 1;
  gameCode.updateGameAndRender = updateGameAndRenderStub;
  gameCode.fillSoundBuffer = fillSoundBufferStub;
  copyfile(GAME_CODE_LIB_NAME, GAME_CODE_TMPLIB_NAME, 0, COPYFILE_ALL);
  gameCode.gameCodeDylib = dlopen(GAME_CODE_TMPLIB_NAME, RTLD_LAZY);
  if (gameCode.gameCodeDylib)
  {
    gameCode.lastModified = newLastModified;
    gameCode.updateGameAndRender = (UpdateGameAndRender *) dlsym(gameCode.gameCodeDylib, "updateGameAndRender");
    if (!gameCode.updateGameAndRender)
    {
      result = 0;
      fprintf(stderr, "Failed to load updateGameAndRender symbol, error: %s\n", dlerror());
    }
    gameCode.fillSoundBuffer = (FillSoundBuffer *) dlsym(gameCode.gameCodeDylib, "fillSoundBuffer");
    if (!gameCode.fillSoundBuffer)
    {
      result = 0;
      fprintf(stderr, "Failed to load fillSoundBuffer symbol, error: %s\n", dlerror());
    }
  }
  else
  {
    result = 0;
    fprintf(stderr, "Failed to load antiqua dylib, error: %s\n", dlerror());
  }
  return result;
}

void unloadGameCode(void)
{
  if (gameCode.gameCodeDylib && dlclose(gameCode.gameCodeDylib) == -1)
  {
    fprintf(stderr, "Failed to close antiqua dylib, error: %s\n", dlerror());
  }
}
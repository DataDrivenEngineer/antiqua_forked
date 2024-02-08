#ifndef _ANTIQUA_INTRINSICS_H_
#define _ANTIQUA_INTRINSICS_H_

#include <math.h>

inline s32 roundReal32ToInt32(r32 v)
{
  s32 result = (s32) roundf(v);
  return result;
}

inline u32 roundReal32ToUInt32(r32 v)
{
  u32 result = (u32) roundf(v);
  return result;
}

inline s32 floorReal32ToInt32(r32 v)
{
  s32 result = (s32) floorf(v);
  return result;
}

inline s32 truncateReal32ToInt32(r32 v)
{
  s32 result = (s32) v;
  return result;
}

typedef struct
{
  b32 found;
  u32 index;
} BitScanResult;

inline BitScanResult findLeastSignificantSetBit(u32 value)
{
  BitScanResult result = {0};

#if COMPILER_LLVM
  result.index = __builtin_ffsl(value) - 1;
  result.found = true;
#else
  for (u32 test = 0; test < 32; ++test)
  {
    if (value & (1 << test))
    {
      result.index = test;
      result.found = true;
      break;
    }
  }
#endif

  return result;
}

#endif

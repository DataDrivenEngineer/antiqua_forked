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

#endif

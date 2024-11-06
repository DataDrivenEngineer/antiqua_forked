#ifndef _ANTIQUA_INTRINSICS_H_
#define _ANTIQUA_INTRINSICS_H_

#include <math.h>

inline s32 signOf(s32 value)
{
  s32 result = value >= 0 ? 1 : -1;
  return result;
}

inline r32 squareRoot(r32 real32)
{
    r32 result;

#if COMPILER_LLVM
    result = __builtin_sqrtf(real32);
#else
    result = sqrtf(real32);
#endif

    return result;
}

inline r32 absoluteValue(r32 real32)
{
  r32 result = fabs(real32);
  return result;
}

inline u32 rotateLeft(u32 value, s32 amount)
{
  u32 result = amount > 0 
    ? (value << amount) | (value >> (32 - amount))
    : (value >> -amount) | (value << (32 + amount));
  
  return result;
}

inline u32 rotateRight(u32 value, s32 amount)
{
  u32 result = amount > 0 
    ? (value >> amount) | (value << (32 - amount))
    : (value << -amount) | (value >> (32 + amount));
  
  return result;
}

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

inline s32 ceilReal32ToInt32(r32 v)
{
  s32 result = (s32) ceilf(v);
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

inline r32 tangent(r32 val)
{
    r32 result;

#if COMPILER_LLVM
    result = __builtin_tanf(val);
#else
    result = tanf(val);
#endif

    return result;
}

inline r32 sine(r32 valRadians)
{
    r32 result;

#if COMPILER_LLVM
    result = __builtin_sinf(valRadians);
#else
    result = sinf(valRadians);
#endif

    return result;
}

inline r32 cosine(r32 valRadians)
{
    r32 result;

#if COMPILER_LLVM
    result = __builtin_cosf(valRadians);
#else
    result = cosf(valRadians);
#endif

    return result;
}

inline r32 arccosine(r32 valDegrees)
{
    r32 result;

    result = acosf(valDegrees);

    return result;
}

inline r32 minimum(r32 one, r32 two)
{
    r32 result;

#if COMPILER_LLVM
    result = __builtin_fmin(one, two);
#else
    result = fminf(one, two);
#endif

    return result;
}

inline r32 maximum(r32 one, r32 two)
{
    r32 result;

#if COMPILER_LLVM
    result = __builtin_fmax(one, two);
#else
    result = fmaxf(one, two);
#endif

    return result;
}

#endif

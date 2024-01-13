#ifndef _TYPES_H_
#define _TYPES_H_

#include <stddef.h>

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

#define EXPORT __attribute__((visibility("default")))

typedef unsigned int b32;

typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;

typedef size_t MemoryIndex;

typedef float r32;
typedef double r64;

#endif

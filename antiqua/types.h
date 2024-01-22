#ifndef _TYPES_H_
#define _TYPES_H_

#if !defined(__cplusplus)
#define MONExternC extern
#else
#define MONExternC extern "C"
#endif

#if !defined(COMPILER_LLVM)
#define COMPILER_LLVM 0
#endif

#if !COMPILER_LLVM
#if defined(__clang__)
#undef COMPILER_LLVM
#define COMPILER_LLVM 1
#endif
#endif

#define EXPORT __attribute__((visibility("default")))

#include <stddef.h>

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

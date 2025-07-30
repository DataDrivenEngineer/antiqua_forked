#ifndef _ANTIQUA_COMMON_FUNCTIONS_H_
#define _ANTIQUA_COMMON_FUNCTIONS_H_

#include <memory.h>

inline u32 stringLength(s8 *str)
{
    u32 result = 0;

    while (*str++)
    {
        result++;
    }

    return result;
}

inline void memCopyAndUpdateOffset(void *dst, void *src, u32 size, u32 *offset)
{
    memcpy(dst, src, size);
    *offset += size;
}

#endif

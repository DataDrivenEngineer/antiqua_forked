#ifndef _ANTIQUA_MATH_H_
#define _ANTIQUA_MATH_H_

#include "types.h"

// NOTE(dima): Vector definitions

struct V2
{
    r32 x, y;
    r32 operator[](s32 index) { return ((&x)[index]); }

    inline V2 &operator*=(r32 a);
    inline V2 &operator+=(V2 a);
};

struct V3
{
    r32 x, y, z;
    r32 operator[](s32 index) { return ((&x)[index]); }

    inline V3 &operator*=(r32 a);
    inline V3 &operator+=(V3 a);
};

struct V4
{
    r32 x, y, z, w;
    r32 operator[](s32 index) { return ((&x)[index]); }

    inline V4 &operator*=(r32 a);
    inline V4 &operator+=(V4 a);
};

// NOTE(dima): Vector factory functions

inline V2 v2(r32 x, r32 y)
{
    V2 result;

    result.x = x;
    result.y = y;

    return result;
}

inline V3 v3(r32 x, r32 y, r32 z)
{
    V3 result;

    result.x = x;
    result.y = y;
    result.z = z;

    return result;
}

inline V4 v4(r32 x, r32 y, r32 z, r32 w)
{
    V4 result;

    result.x = x;
    result.y = y;
    result.z = z;
    result.w = w;

    return result;
}

inline V3 v3(V2 a, r32 val)
{
    V3 result;

    result.x = a.x;
    result.y = a.y;
    result.z = val;

    return result;
}

inline V4 v4(V3 a, r32 val)
{
    V4 result;

    result.x = a.x;
    result.y = a.y;
    result.z = a.z;
    result.w = val;

    return result;
}

inline V2 v2(V3 a)
{
    V2 result;

    result.x = a.x;
    result.y = a.y;

    return result;
}

inline V3 v3(V4 a)
{
    V3 result;

    result.x = a.x;
    result.y = a.y;
    result.z = a.z;

    return result;
}

// NOTE(dima): Operator override: multiply by scalar

inline V2 operator*(r32 a, V2 b)
{
    V2 result;

    result.x = a * b.x;
    result.y = a * b.y;

    return result;
}

inline V2 operator*(V2 b, r32 a)
{
    V2 result;

    result.x = a * b.x;
    result.y = a * b.y;

    return result;
}

inline V3 operator*(r32 a, V3 b)
{
    V3 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return result;
}

inline V3 operator*(V3 b, r32 a)
{
    V3 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;

    return result;
}

inline V4 operator*(r32 a, V4 b)
{
    V4 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    result.w = a * b.w;

    return result;
}

inline V4 operator*(V4 b, r32 a)
{
    V4 result;

    result.x = a * b.x;
    result.y = a * b.y;
    result.z = a * b.z;
    result.w = a * b.w;

    return result;
}

// NOTE(dima): Operator override: multiply equal by scalar

inline V2 &V2::
operator*=(r32 a)
{
    *this = a * *this;

    return *this;
}

inline V3 &V3::
operator*=(r32 a)
{
    *this = a * *this;

    return *this;
}

inline V4 &V4::
operator*=(r32 a)
{
    *this = a * *this;

    return *this;
}

// NOTE(dima): Operator override: subtract a vector

inline V2 operator-(V2 a, V2 b)
{
    V2 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;

    return result;
}

inline V3 operator-(V3 a, V3 b)
{
    V3 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;

    return result;
}

inline V4 operator-(V4 a, V4 b)
{
    V4 result;

    result.x = a.x - b.x;
    result.y = a.y - b.y;
    result.z = a.z - b.z;
    result.w = a.w - b.w;

    return result;
}

// NOTE(dima): Operator override: add a vector

inline V2 operator+(V2 a, V2 b)
{
    V2 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;

    return result;
}

inline V3 operator+(V3 a, V3 b)
{
    V3 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;

    return result;
}

inline V4 operator+(V4 a, V4 b)
{
    V4 result;

    result.x = a.x + b.x;
    result.y = a.y + b.y;
    result.z = a.z + b.z;
    result.w = a.w + b.w;

    return result;
}

// NOTE(dima): Dot product

inline r32 dot(V2 a, V2 b)
{
    r32 result;

    result = a.x * b.x + a.y * b.y;

    return result;
}

inline r32 dot(V3 a, V3 b)
{
    r32 result;

    result = a.x * b.x + a.y * b.y + a.z * b.z;

    return result;
}

inline r32 dot(V4 a, V4 b)
{
    r32 result;

    result = a.x * b.x + a.y * b.y + a.z * b.z + a.w * b.w;

    return result;
}

// NOTE(dima): Cross product

inline V3 cross(V3 a, V3 b)
{
    V3 result;

    result.x = a.y * b.z - a.z * b.y;
    result.y = a.z * b.x - a.x * b.z;
    result.z = a.x * b.y - a.y * b.x;

    return result;
}

// NOTE(dima): Vector square length

inline r32 squareLength(V2 a)
{
    r32 result = dot(a, a);

    return result;
}

inline r32 squareLength(V3 a)
{
    r32 result = dot(a, a);

    return result;
}

inline r32 squareLength(V4 a)
{
    r32 result = dot(a, a);

    return result;
}

// NOTE(dima): Vector length

inline r32 length(V2 a)
{
    r32 result = dot(a, a);
#if COMPILER_LLVM
    result = __builtin_sqrt(result);
#else
#include <math.h>
    result = sqrt(result);
#endif
    return result;
}

inline r32 length(V3 a)
{
    r32 result = dot(a, a);
#if COMPILER_LLVM
    result = __builtin_sqrt(result);
#else
#include <math.h>
    result = sqrt(result);
#endif
    return result;
}

inline r32 length(V4 a)
{
    r32 result = dot(a, a);
#if COMPILER_LLVM
    result = __builtin_sqrt(result);
#else
#include <math.h>
    result = sqrt(result);
#endif
    return result;
}

// NOTE(dima): Matrix definitions

// NOTE(dima): all matrices are in column-major order

struct M33
{
    r32 cols[9];
    V3 *operator[](s32 index)
    {
        ASSERT(index >= 0 && index < 3);
        r32 *col = cols + index * 3;

        return (V3 *) col;
    }
};

struct M44
{
    r32 cols[16];
    V4 *operator[](s32 index)
    {
        ASSERT(index >= 0 && index < 4);
        r32 *col = cols + index * 4;

        return (V4 *) col;
    }

    V3 xyz(s32 index)
    {
        ASSERT(index >= 0 && index < 4);
        V4 *v4 = (V4 *) (*this)[index];
        V3 result = v3(*v4);

        return result;
    }
};

// NOTE(dima): Matrix transpose

inline M33 transpose(M33 a)
{
    M33 result;
    for (int i = 0; i < 3; i++)
    {
        V3 *col = a[i];
        result.cols[i + 0] = col->x;
        result.cols[i + 3] = col->y;
        result.cols[i + 6] = col->z;
    }

    return result;
}

inline M44 transpose(M44 a)
{
    M44 result;
    for (int i = 0; i < 4; i++)
    {
        V4 *col = a[i];
        result.cols[i + 0] = col->x;
        result.cols[i + 4] = col->y;
        result.cols[i + 8] = col->z;
        result.cols[i + 12] = col->w;
    }

    return result;
}

// NOTE(dima): Operator override: multiply matrix by matrix

inline M33 operator*(M33 a, M33 b)
{
    M33 result;

    M33 aT = transpose(a);

    for (int i = 0; i < 3; i++)
    {
        r32 dot0 = dot(*aT[0], *b[i]);
        r32 dot1 = dot(*aT[1], *b[i]);
        r32 dot2 = dot(*aT[2], *b[i]);

        result.cols[i * 3 + 0] = dot0;
        result.cols[i * 3 + 1] = dot1;
        result.cols[i * 3 + 2] = dot2;
    }

    return result;
}

inline M44 operator*(M44 a, M44 b)
{
    M44 result;

    M44 aT = transpose(a);

    for (int i = 0; i < 4; i++)
    {
        r32 dot0 = dot(*aT[0], *b[i]);
        r32 dot1 = dot(*aT[1], *b[i]);
        r32 dot2 = dot(*aT[2], *b[i]);
        r32 dot3 = dot(*aT[3], *b[i]);

        result.cols[i * 4 + 0] = dot0;
        result.cols[i * 4 + 1] = dot1;
        result.cols[i * 4 + 2] = dot2;
        result.cols[i * 4 + 3] = dot3;
    }

    return result;
}

#endif

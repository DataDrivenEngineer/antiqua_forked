#ifndef _ANTIQUA_MATH_H_
#define _ANTIQUA_MATH_H_

#include "types.h"
#include "antiqua_intrinsics.h"

#include "antiqua_math_generated.h"

// NOTE(dima): 2D cross product

inline r32 crossXZ(V3 a, V3 b)
{
    r32 result;

    result = a.x*b.z - a.z*b.x;

    return result;
}
// NOTE(dima): Operator override: multiply 4x4 matrix by 4x1 vector

inline V3 operator*(M33 a, V3 b)
{
    V3 result;

    M33 aT = transpose(&a);

    result.x = dot(*aT[0], b);
    result.y = dot(*aT[1], b);
    result.z = dot(*aT[2], b);

    return result;
}

inline V4 operator*(M44 a, V4 b)
{
    V4 result;

    M44 aT = transpose(&a);

    result.x = dot(*aT[0], b);
    result.y = dot(*aT[1], b);
    result.z = dot(*aT[2], b);
    result.w = dot(*aT[3], b);

    return result;
}

// NOTE(dima): Inverse of 4x4 matrix

inline r32 determinant(M44 *m)
{
    r32 result = m->cols[0] * (m->cols[5] * (m->cols[10] * m->cols[15] - m->cols[11] * m->cols[14])
                               - m->cols[9] * (m->cols[6] * m->cols[15] - m->cols[7] * m->cols[14])
                               + m->cols[13] * (m->cols[6] * m->cols[11] - m->cols[7] * m->cols[10]))
                 - m->cols[4] * (m->cols[1] * (m->cols[10] * m->cols[15] - m->cols[11] * m->cols[14])
                              - m->cols[9] * (m->cols[2] * m->cols[15] - m->cols[3] * m->cols[14])
                              + m->cols[13] * (m->cols[2] * m->cols[11] - m->cols[3] * m->cols[10]))
                 + m->cols[8] * (m->cols[1] * (m->cols[6] * m->cols[15] - m->cols[7] * m->cols[14])
                              - m->cols[5] * (m->cols[2] * m->cols[15] - m->cols[3] * m->cols[14])
                              + m->cols[13] * (m->cols[2] * m->cols[7] - m->cols[3] * m->cols[6]))
                 - m->cols[12] * (m->cols[1] * (m->cols[6] * m->cols[11] - m->cols[7] * m->cols[10])
                              - m->cols[5] * (m->cols[2] * m->cols[11] - m->cols[3] * m->cols[10])
                              + m->cols[9] * (m->cols[2] * m->cols[7] - m->cols[3] * m->cols[6]));

    return result;
}

inline M44 inverse(M44 *m)
{
    M44 result;

    r32 det = determinant(m);

    result.cols[0] = (1 / det) 
                     * (m->cols[5] * (m->cols[10] * m->cols[15] - m->cols[11] * m->cols[14])
                        - m->cols[9] * (m->cols[6] * m->cols[15] - m->cols[7] * m->cols[14])
                        + m->cols[13] * (m->cols[6] * m->cols[11] - m->cols[7] * m->cols[10]));
    result.cols[1] = - (1 / det) 
                     * (m->cols[1] * (m->cols[10] * m->cols[15] - m->cols[11] * m->cols[14])
                        - m->cols[9] * (m->cols[2] * m->cols[15] - m->cols[3] * m->cols[14])
                        + m->cols[13] * (m->cols[2] * m->cols[11] - m->cols[3] * m->cols[10]));
    result.cols[2] = (1 / det) 
                     * (m->cols[1] * (m->cols[6] * m->cols[15] - m->cols[7] * m->cols[14])
                        - m->cols[5] * (m->cols[2] * m->cols[15] - m->cols[3] * m->cols[14])
                        + m->cols[13] * (m->cols[2] * m->cols[7] - m->cols[3] * m->cols[6]));
    result.cols[3] = - (1 / det) 
                     * (m->cols[1] * (m->cols[6] * m->cols[11] - m->cols[7] * m->cols[10])
                        - m->cols[5] * (m->cols[2] * m->cols[11] - m->cols[3] * m->cols[10])
                        + m->cols[9] * (m->cols[2] * m->cols[7] - m->cols[3] * m->cols[6]));
    result.cols[4] = - (1 / det) 
                     * (m->cols[4] * (m->cols[10] * m->cols[15] - m->cols[11] * m->cols[14])
                        - m->cols[8] * (m->cols[6] * m->cols[15] - m->cols[7] * m->cols[14])
                        + m->cols[12] * (m->cols[6] * m->cols[11] - m->cols[7] * m->cols[10]));
    result.cols[5] = (1 / det) 
                     * (m->cols[0] * (m->cols[10] * m->cols[15] - m->cols[11] * m->cols[14])
                        - m->cols[8] * (m->cols[2] * m->cols[15] - m->cols[3] * m->cols[14])
                        + m->cols[12] * (m->cols[2] * m->cols[11] - m->cols[3] * m->cols[10]));
    result.cols[6] = - (1 / det) 
                     * (m->cols[0] * (m->cols[6] * m->cols[15] - m->cols[7] * m->cols[14])
                        - m->cols[4] * (m->cols[2] * m->cols[15] - m->cols[3] * m->cols[14])
                        + m->cols[12] * (m->cols[2] * m->cols[7] - m->cols[3] * m->cols[6]));
    result.cols[7] = (1 / det) 
                     * (m->cols[0] * (m->cols[6] * m->cols[11] - m->cols[7] * m->cols[10])
                        - m->cols[4] * (m->cols[2] * m->cols[11] - m->cols[3] * m->cols[10])
                        + m->cols[8] * (m->cols[2] * m->cols[7] - m->cols[3] * m->cols[6]));
    result.cols[8] = (1 / det) 
                     * (m->cols[4] * (m->cols[9] * m->cols[15] - m->cols[11] * m->cols[13])
                        - m->cols[8] * (m->cols[5] * m->cols[15] - m->cols[7] * m->cols[13])
                        + m->cols[12] * (m->cols[5] * m->cols[11] - m->cols[7] * m->cols[9]));
    result.cols[9] = - (1 / det) 
                     * (m->cols[0] * (m->cols[9] * m->cols[15] - m->cols[11] * m->cols[13])
                        - m->cols[8] * (m->cols[1] * m->cols[15] - m->cols[3] * m->cols[13])
                        + m->cols[12] * (m->cols[1] * m->cols[11] - m->cols[3] * m->cols[9]));
    result.cols[10] = (1 / det) 
                     * (m->cols[0] * (m->cols[5] * m->cols[15] - m->cols[7] * m->cols[13])
                        - m->cols[4] * (m->cols[1] * m->cols[15] - m->cols[3] * m->cols[13])
                        + m->cols[12] * (m->cols[1] * m->cols[7] - m->cols[3] * m->cols[5]));
    result.cols[11] = - (1 / det) 
                     * (m->cols[0] * (m->cols[5] * m->cols[11] - m->cols[7] * m->cols[9])
                        - m->cols[4] * (m->cols[1] * m->cols[11] - m->cols[3] * m->cols[9])
                        + m->cols[8] * (m->cols[1] * m->cols[7] - m->cols[3] * m->cols[5]));
    result.cols[12] = - (1 / det) 
                     * (m->cols[4] * (m->cols[9] * m->cols[14] - m->cols[10] * m->cols[13])
                        - m->cols[8] * (m->cols[5] * m->cols[14] - m->cols[6] * m->cols[13])
                        + m->cols[12] * (m->cols[5] * m->cols[10] - m->cols[6] * m->cols[9]));
    result.cols[13] = (1 / det) 
                     * (m->cols[0] * (m->cols[9] * m->cols[14] - m->cols[10] * m->cols[13])
                        - m->cols[8] * (m->cols[1] * m->cols[14] - m->cols[2] * m->cols[13])
                        + m->cols[12] * (m->cols[1] * m->cols[10] - m->cols[2] * m->cols[9]));
    result.cols[14] = - (1 / det) 
                     * (m->cols[0] * (m->cols[5] * m->cols[14] - m->cols[6] * m->cols[13])
                        - m->cols[4] * (m->cols[1] * m->cols[14] - m->cols[2] * m->cols[13])
                        + m->cols[12] * (m->cols[1] * m->cols[6] - m->cols[2] * m->cols[5]));
    result.cols[15] = (1 / det) 
                     * (m->cols[0] * (m->cols[5] * m->cols[10] - m->cols[6] * m->cols[9])
                        - m->cols[4] * (m->cols[1] * m->cols[10] - m->cols[2] * m->cols[9])
                        + m->cols[8] * (m->cols[1] * m->cols[6] - m->cols[2] * m->cols[5]));

    return result;
}

// NOTE(dima): Quaternions

struct Q
{
    r32 x, y, z, w;
    r32 operator[](s32 index) { return ((&x)[index]); }
};

inline Q q(V3 v, r32 a)
{
    Q result;

    r32 aRad = RADIANS(a/2);
    r32 cosA = cosine(aRad);
    r32 sinA = sine(aRad);

    result.x = v.x * sinA;
    result.y = v.y * sinA;
    result.z = v.z * sinA;
    result.w = cosA;

    return result;
}

inline Q q(r32 x, r32 y, r32 z, r32 a)
{
    Q result;

    r32 aRad = RADIANS(a/2);
    r32 cosA = cosine(aRad);
    r32 sinA = sine(aRad);

    result.x = x * sinA;
    result.y = y * sinA;
    result.z = z * sinA;
    result.w = cosA;

    return result;
}

inline void normalize(Q *a)
{
    r32 length = squareRoot(a->x * a->x + a->y * a->y + a->z * a->z + a->w * a->w);
    a->x /= length;
    a->y /= length;
    a->z /= length;
    a->w /= length;
}

inline Q conjugate(Q a)
{
    Q result;

    result.x = -a.x;
    result.y = -a.y;
    result.z = -a.z;
    result.w = a.w;

    return result;
}

inline Q operator*(Q q, V3 v)
{
    Q result;

    result.x = (q.w * v.x) + (q.y * v.z) - (q.z * v.y);
    result.y = (q.w * v.y) + (q.z * v.x) - (q.x * v.z);
    result.z = (q.w * v.z) + (q.x * v.y) - (q.y * v.x);
    result.w = - (q.x * v.x) - (q.y * v.y) - (q.z * v.z);

    return result;
}

inline Q operator*(Q l, Q r)
{
    Q result;

    result.x = (l.x * r.w) + (l.w * r.x) + (l.y * r.z) - (l.z * r.y);
    result.y = (l.y * r.w) + (l.w * r.y) + (l.z * r.x) - (l.x * r.z);
    result.z = (l.z * r.w) + (l.w * r.z) + (l.x * r.y) - (l.y * r.x);
    result.w = (l.w * r.w) - (l.x * r.x) - (l.y * r.y) - (l.z * r.z);

    return result;
}

inline void rotate(V3 *vectorToRotate, V3 vectorOfRotation, r32 rotationAngleInDegrees)
{
    Q rotationQuaternion = q(vectorOfRotation, rotationAngleInDegrees);
    Q rotationQuaternionConjugate = conjugate(rotationQuaternion);
    Q rotated = rotationQuaternion * (*vectorToRotate) * rotationQuaternionConjugate;

    vectorToRotate->x = rotated.x;
    vectorToRotate->y = rotated.y;
    vectorToRotate->z = rotated.z;

    normalize(vectorToRotate);
}

// NOTE(dima): Miscellaneous functions

inline r32 square(r32 val)
{
    r32 result;

    result = val * val;

    return result;
}

inline u32 asciiToU32OverflowUnsafe(s8 *s, u32 length)
{
    u32 result = 0;
    u32 prevResult;

    for (u32 idx = 0;
         idx < length;
         ++idx)
    {
        ASSERT(s[idx] >= '0' && s[idx] <= '9');

        u32 digit = s[idx] - '0';

        prevResult = result;
        result = result * 10 + digit;
        // NOTE(dima): make sure there was no overflow
        ASSERT(result >= prevResult);
    }

    return result;
}

inline r32 asciiToR32(s8 *s)
{
    r32 result;

    result = strtof((const char *)s, 0);

    return result;
}

#endif

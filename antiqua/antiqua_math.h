#ifndef _ANTIQUA_MATH_H_
#define _ANTIQUA_MATH_H_

#include "types.h"
#include "antiqua_intrinsics.h"

#include "antiqua_math_generated.h"

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

#endif

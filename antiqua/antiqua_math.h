#ifndef _ANTIQUA_MATH_H_
#define _ANTIQUA_MATH_H_

// V2

struct v2
{
  r32 x, y;
  r32 operator[](s32 index) { return ((&x)[index]); }

  inline v2 &operator*=(r32 a);
  inline v2 &operator+=(v2 a);
};

inline v2 V2(r32 x, r32 y)
{
  v2 result;

  result.x = x;
  result.y = y;

  return result;
}

inline v2 operator*(r32 a, v2 b)
{
  v2 result;

  result.x = a * b.x;
  result.y = a * b.y;

  return result;
}

inline v2 operator*(v2 b, r32 a)
{
  v2 result;

  result.x = a * b.x;
  result.y = a * b.y;

  return result;
}

inline v2 &v2::
operator*=(r32 a)
{
  *this = a * *this;

  return *this;
}

inline v2 operator-(v2 a)
{
  v2 result;

  result.x = -a.x;
  result.y = -a.y;
  
  return result;
}

inline v2 operator+(v2 a, v2 b)
{
  v2 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;

  return result;
}

inline v2 &v2::
operator+=(v2 a)
{
  *this = a + *this;

  return *this;
}

inline v2 operator-(v2 a, v2 b)
{
  v2 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;

  return result;
}

// V4

struct v4
{
  r32 x, y, z, w;
  r32 operator[](s32 index) { return ((&x)[index]); }

//  inline v4 &operator*=(r32 a);
//  inline v4 &operator+=(v4 a);
};

inline v4 V4(r32 x, r32 y, r32 z, r32 w)
{
  v4 result;

  result.x = x;
  result.y = y;
  result.z = z;
  result.w = w;

  return result;
}

inline r32 square(r32 a)
{
  r32 result = a * a;

  return result;
}

inline r32 inner(v2 a, v2 b)
{
  r32 result = a.x * b.x + a.y * b.y;

  return result;
}

inline r32 lengthSq(v2 a)
{
  r32 result = inner(a, a);

  return result;
}

typedef struct
{
  v2 min;
  v2 max;
} Rectangle2;

inline Rectangle2 rectMinMax(v2 min, v2 max)
{
  Rectangle2 result;

  result.min = min;
  result.max = max;

  return result;
}

inline Rectangle2 rectMinDim(v2 min, v2 dim)
{
  Rectangle2 result;

  result.min = min;
  result.max = min + dim;

  return result;
}

inline Rectangle2 rectCenterHalfDim(v2 center, v2 halfDim)
{
  Rectangle2 result;

  result.min = center - halfDim;;
  result.max = center + halfDim;

  return result;
}

inline Rectangle2 rectCenterDim(v2 center, v2 dim)
{
  Rectangle2 result = rectCenterHalfDim(center, 0.5f * dim);

  return result;
}

inline b32 isInRectangle(Rectangle2 rectangle, v2 test)
{
  b32 result = test.x >= rectangle.min.x &&
               test.y >= rectangle.min.y &&
               test.x < rectangle.max.x &&
               test.y < rectangle.max.y;

  return result;
}

#endif

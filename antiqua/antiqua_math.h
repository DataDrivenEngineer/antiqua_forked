#ifndef _ANTIQUA_MATH_H_
#define _ANTIQUA_MATH_H_

struct V2
{
  r32 x, y;
  r32 operator[](s32 index) { return ((&x)[index]); }

  inline V2 &operator*=(r32 a);
  inline V2 &operator+=(V2 a);
};

inline V2 v2(r32 x, r32 y)
{
  V2 result;

  result.x = x;
  result.y = y;

  return result;
}

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

inline V2 &V2::
operator*=(r32 a)
{
  *this = a * *this;

  return *this;
}

inline V2 operator-(V2 a)
{
  V2 result;

  result.x = -a.x;
  result.y = -a.y;
  
  return result;
}

inline V2 operator+(V2 a, V2 b)
{
  V2 result;

  result.x = a.x + b.x;
  result.y = a.y + b.y;

  return result;
}

inline V2 &V2::
operator+=(V2 a)
{
  *this = a + *this;

  return *this;
}

inline V2 operator-(V2 a, V2 b)
{
  V2 result;

  result.x = a.x - b.x;
  result.y = a.y - b.y;

  return result;
}

inline r32 square(r32 a)
{
  r32 result = a * a;

  return result;
}

inline r32 inner(V2 a, V2 b)
{
  r32 result = a.x * b.x + a.y * b.y;

  return result;
}

inline r32 lengthSq(V2 a)
{
  r32 result = inner(a, a);

  return result;
}

typedef struct
{
  V2 min;
  V2 max;
} Rectangle2;

inline Rectangle2 rectMinMax(V2 min, V2 max)
{
  Rectangle2 result;

  result.min = min;
  result.max = max;

  return result;
}

inline Rectangle2 rectMinDim(V2 min, V2 dim)
{
  Rectangle2 result;

  result.min = min;
  result.max = min + dim;

  return result;
}

inline Rectangle2 rectCenterHalfDim(V2 center, V2 halfDim)
{
  Rectangle2 result;

  result.min = center - halfDim;;
  result.max = center + halfDim;

  return result;
}

inline Rectangle2 rectCenterDim(V2 center, V2 dim)
{
  Rectangle2 result = rectCenterHalfDim(center, 0.5f * dim);

  return result;
}

inline b32 isInRectangle(Rectangle2 rectangle, V2 test)
{
  b32 result = test.x >= rectangle.min.x &&
               test.y >= rectangle.min.y &&
               test.x < rectangle.max.x &&
               test.y < rectangle.max.y;

  return result;
}

#endif

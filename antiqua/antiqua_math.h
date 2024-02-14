#ifndef _ANTIQUA_MATH_H_
#define _ANTIQUA_MATH_H_

struct V2
{
  r32 x, y;
  r32 operator[](s32 index) { return ((&x)[index]); }

  inline V2 &operator*=(r32 a);
  inline V2 &operator+=(V2 a);
};

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

#endif

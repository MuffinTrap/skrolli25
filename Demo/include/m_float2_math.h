#ifndef M_FLOAT2_MATH_H
#define M_FLOAT2_MATH_H

#include <m_math.h>

float2 M_ROTATE2(float2 v, float radians);

void M_ROTATE2_PTR(float2* v, float radians);

#define M_SCALE2(dest, A, scalar) { (dest).x = (A).x * scalar; (dest).y = (A).y * scalar;}

#endif

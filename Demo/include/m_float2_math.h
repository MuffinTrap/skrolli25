#ifndef M_FLOAT2_MATH_H
#define M_FLOAT2_MATH_H

#include <m_math.h>

float2 M_ROTATE2(float2 v, float radians);

void M_ROTATE2_PTR(float2* v, float radians);

float2 direction_2d(float2 A, float2 B);

#define M_RIGHT_ANGLE2(dest, A) { (dest).x = (A).y * 1.0f; (dest).y = (A).x * -1.0f; }

#define M_SCALE2(dest, A, scalar) { (dest).x = (A).x * scalar; (dest).y = (A).y * scalar;}

#endif

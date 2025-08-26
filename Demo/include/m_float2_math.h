#ifndef M_FLOAT2_MATH_H
#define M_FLOAT2_MATH_H

#include <m_math.h>

float2 M_ROTATE2(float2 v, float radians) {
	float xt = v.x*cos(radians) - v.y*sin(radians);
	float yt = v.x*sin(radians) + v.y*cos(radians);
    float2 rotated = {xt, yt};
	return rotated;
}

void M_ROTATE2_PTR(float2* v, float radians) {
	float xt = v->x*cos(radians) - v->y*sin(radians);
	float yt = v->x*sin(radians) + v->y*cos(radians);
	v->x = xt;
    v->y = yt;
}

#define M_SCALE2(dest, A, scalar) { (dest).x = (A).x * scalar; (dest).y = (A).y * scalar;}

#endif

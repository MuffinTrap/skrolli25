#include "m_float2_math.h"

#include <math.h>

float2 M_ROTATE2(float2 v, float radians) {
	const float c = cos(radians);
	const float s = sin(radians);
	const float xt = v.x*c - v.y*s;
	const float yt = v.x*s + v.y*c;
    float2 rotated = {xt, yt};
	return rotated;
}

void M_ROTATE2_PTR(float2* v, float radians) {
	const float c = cos(radians);
	const float s = sin(radians);
	const float xt = v->x*c - v->y*s;
	const float yt = v->x*s + v->y*c;
	v->x = xt;
    v->y = yt;
}

float2 direction_2d(float2 A, float2 B)
{
    float2 dir;
    M_SUB2(dir, B, A);
    M_NORMALIZE2(dir, dir);
    return dir;
}

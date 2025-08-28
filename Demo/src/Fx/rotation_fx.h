#ifndef ROTATION_FX_H
#define ROTATION_FX_H
#include "pointlist.h"
#include <m_math.h>

void rotation_fx(float2 center, float radius_scale, float speed, float progress_normalized, float3 color1, float3 color2, PointList* outerCenters, PointList* recursive_list);

#endif

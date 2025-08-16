#ifndef FLAKE_WHEEL_FX_H
#define FLAKE_WHEEL_FX_H


void flake_wheel_fx(float2 center, float shape_radius, float pattern_radius,
                    float shape_rotation_deg, float pattern_rotation_deg,
                    float time, PointList* cornerlist, PointList* recursive_list, PointList* local_list) ;
#endif

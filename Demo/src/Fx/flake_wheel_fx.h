#ifndef FLAKE_WHEEL_FX_H
#define FLAKE_WHEEL_FX_H

#include "gradient.h"
#include "koch_flake.h"
#include "../Ziz/mesh.h"

void flake_wheel_fx(struct Mesh* flake,
                    float pattern_radius,
                    float pattern_radius_outer,
                    float shape_rotation_deg,
                    float pattern_rotation_deg,
                    float pattern_rotation_deg_outer,
                    struct Gradient* gradient,
                    float base_color_stop,
                    float ring_color_offset,
                    float shape_color_offset);
#endif

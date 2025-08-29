#ifndef FLAKE_WHEEL_FX_H
#define FLAKE_WHEEL_FX_H

#include "gradient.h"
#include "koch_flake.h"
#include "../Ziz/mesh.h"

void flake_wheel_fx(struct Mesh* flake,
                    float pattern_radius,
                    float shape_rotation_deg,
                    float pattern_rotation_deg,
                    float time,
                    struct Gradient* gradient,
                    float color_stop);
#endif

#ifndef ROTATION_FX_H
#define ROTATION_FX_H
#include "../Ziz/mesh.h"
#include "gradient.h"
#include <m_math.h>

void rotation_fx(struct Mesh* flake,
                 float radius_scale,
                 float speed,
                 float progress_normalized,
                 struct Gradient* gradient,
                 float color_1_stop,
                 float color_2_stop);

#endif

#ifndef ROTATION_FX_H
#define ROTATION_FX_H
#include "../Ziz/mesh.h"
#include "gradient.h"
#include <m_math.h>

void rotation_fx(
    struct Mesh* flake3,
    struct Mesh* flake4,
                 float radius_scale,
                 float progress_normalized,
                 struct Gradient* gradient,
                 float foreground_stop,
                 float background_stop);

#endif

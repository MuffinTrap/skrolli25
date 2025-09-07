#ifndef ROTATION_FX_H
#define ROTATION_FX_H
#include "../Ziz/mesh.h"
#include "gradient.h"
#include <m_math.h>

void rotation_fx(
    struct Mesh* flake4,
                 float radius_scale,
                 float progress_normalized,
                 color3 fore,
                 color3 back);

#endif

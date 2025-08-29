#ifndef FLAKE_WHEEL_FX_C
#define FLAKE_WHEEL_FX_C

#include <math.h>
#include <opengl_include.h>
#include "koch_flake.h"
#include "gradient.h"
#include "../Ziz/screenprint.h"

void flake_wheel_fx(struct Mesh* flake,
                    float pattern_radius,
                    float shape_rotation_deg,
                    float pattern_rotation_deg,
                    float time,
                    struct Gradient* gradient,
                    float color_stop
                    )
{
    float2 zero2 = {0.0f, 0.0f};
    short recursion = 2;
    float2 cornerlist[6];
    get_corners(zero2, 6, pattern_radius, pattern_rotation_deg + time/2, cornerlist);
    //screenprintf("%.1f, %.1f\n", center.x, center.y);
    Gradient_glColor(gradient, color_stop);
    for (int i = 0; i < 6; i++)
    {
        float2 hexpoints[6];
        get_corners(cornerlist[i], 6, pattern_radius, pattern_rotation_deg + sin(time/100) * 20, hexpoints);
        for (int p = 0; p < 6; p++)
        {
            glPushMatrix();
            glTranslatef(
                hexpoints[p].x,
                hexpoints[p].y,
                0.0f);
            glRotatef(shape_rotation_deg+time, 0.0f, 0.0f, 1.0f);
            glScalef(pattern_radius, pattern_radius, 1.0f);
            Mesh_DrawArrays(flake);
            //draw_snowflake(zero2, pattern_radius/6.0f, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
            glPopMatrix();
        }

    }
}

#endif

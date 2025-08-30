#ifndef FLAKE_WHEEL_FX_C
#define FLAKE_WHEEL_FX_C

#include <math.h>
#include <opengl_include.h>
#include "koch_flake.h"
#include "gradient.h"
#include "../Ziz/screenprint.h"

void flake_wheel_fx(struct Mesh* flake,
                    float pattern_radius,
                    float pattern_radius_outer,
                    float shape_rotation_deg,
                    float pattern_rotation_deg,
                    float pattern_rotation_deg_outer,
                    struct Gradient* gradient,
                    float base_color_stop,
                    float ring_color_offset,
                    float shape_color_offset
                    )
{
    float2 zero2 = {0.0f, 0.0f};
    short recursion = 2;
    float2 cornerlist[6];
    get_corners(zero2, 6, pattern_radius, pattern_rotation_deg, cornerlist);
    //screenprintf("%.1f, %.1f\n", center.x, center.y);
    for (int i = 0; i < 6; i++)
    {
        float2 hexpoints[6];
        get_corners(cornerlist[i], 6, pattern_radius_outer, pattern_rotation_deg_outer, hexpoints);
        for (int p = 0; p < 6; p++)
        {
            glPushMatrix();
            glTranslatef(
                hexpoints[p].x,
                hexpoints[p].y,
                0.0f);
            glRotatef(shape_rotation_deg, 0.0f, 0.0f, 1.0f);
            //glScalef(pattern_radius/6.0f, pattern_radius/6.0f, 1.0f);
            Gradient_glColor(gradient, base_color_stop + ring_color_offset * i + shape_color_offset * p);
            Mesh_DrawArrays(flake, DrawTriangles);
            //draw_snowflake(zero2, pattern_radius/6.0f, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
            glPopMatrix();
        }

    }
}

#endif

#ifndef FLAKE_WHEEL_FX_C
#define FLAKE_WHEEL_FX_C

#include <math.h>
#include "koch_flake.h"
#include <opengl_include.h>
#include "../Ziz/screenprint.h"

void flake_wheel_fx(float2 center, float shape_radius, float pattern_radius,
                    float shape_rotation_deg, float pattern_rotation_deg,
                    float time,
                    PointList* cornerlist, PointList* recursive_list, PointList* local_list)
{
    float2 zero2 = {0.0f, 0.0f};
    short recursion = 2;
    cornerlist = get_corners(center, 6, pattern_radius, pattern_rotation_deg + time/2, cornerlist);
    screenprintf("%.1f, %.1f\n", center.x, center.y);
    for (int i = 0; i < 6; i++)
    {
        PointList_clear(local_list);
        local_list = get_corners(cornerlist->points[i], 6, pattern_radius, pattern_rotation_deg + sin(time/100) * 20, local_list);
        float2 hexpoints[6];
        hexpoints[0]= local_list->points[0];
        hexpoints[1]= local_list->points[1];
        hexpoints[2]= local_list->points[2];
        hexpoints[3]= local_list->points[3];
        hexpoints[4]= local_list->points[4];
        hexpoints[5]= local_list->points[5];
        for (int p = 0; p < 6; p++)
        {
            glPushMatrix();
            glTranslatef(
                hexpoints[p].x,
                hexpoints[p].y,
                0.0f);
            glRotatef(shape_rotation_deg+time, 0.0f, 0.0f, 1.0f);
            glColor3f(0.7f + cos(time * (p+i)/40)*0.5f, 0.6f + sin(time/(p-i*2)/10) * 0.3f, 0.2f + sin(time/30)*0.15f);
            draw_snowflake(zero2, shape_radius/6.0f, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
            glPopMatrix();
        }

    }
}

#endif

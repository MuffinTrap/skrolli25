#ifndef ROTATION_FX_C
#define ROTATION_FX_C

#include "rotation_fx.h"
//static float progression1 = 0.0f;
//static float progression2 = 0.0f;

void rotation_fx(float2 center, float radius_scale, float speed, float progress_normalized, float3 color1, float3 color2, PointList* outerCenters, PointList* recursive_list, PointList* local_list)
{
    progress_normalized = progress_normalized - floor(progress_normalized);
    short fx_recursion = 3;
    screenprintf("Prog N %.1f\n", progress_normalized);
    screenprintf("Wheel center %.1f %.1f\n", center.x, center.y);

    float move_amount = (360)/12.0f;
    float2 zero2 = {0.0f, 0.0f};
    if (progress_normalized <= 0.5f)
    {
        float size = 41.0f * radius_scale;
        short recursion = fx_recursion-1;
        // Quadratic
        float pn = progress_normalized * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);

        // Extra background flake
        glColor3f(color1.x, color1.y, color1.z);
            glPushMatrix();
                //screenprintf("ring center %.1f %.1f\n", ringcenter.x, ringcenter.y);
                glTranslatef(center.x, center.y, 0.0f);
                glRotatef(progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);
                draw_snowflake(zero2, size*1.5f, recursion+1, 60.0f, 1.0f/3.0f, 1.0f, recursive_list, local_list);
        glPopMatrix();

        glColor3f(color2.x, color2.y, color2.z);
        outerCenters = get_corners(center, 6, size, progression * move_amount, outerCenters);
        for (int i = 0; i < 6; i++)
        {
            float2 ringcenter = outerCenters->points[i];
            glPushMatrix();
                //screenprintf("ring center %.1f %.1f\n", ringcenter.x, ringcenter.y);
                glTranslatef(ringcenter.x, ringcenter.y, 0.0f);
                glRotatef(progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);

                draw_snowflake(zero2, size/2.0f, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list, local_list);

            glPopMatrix();
        }
    }
    else if (progress_normalized > 0.5f)
    {
        short recursion = fx_recursion;
        float size = 59.0f * radius_scale;
        float inner_size = 35.0f * radius_scale;

        // Quadratic
        float pn = (progress_normalized - 0.5f) * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);
        glColor3f(color2.x, color2.y, color2.z);
        glPushMatrix();
            glTranslatef(center.x, center.y, 0.0f);

            glPushMatrix();
            glRotatef(30.0f + progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);
            draw_snowflake(zero2, size, recursion,
                        60.0f,
                        1.0f/3.0f, 1.0f,
                        recursive_list, local_list);
            glPopMatrix();

            glPushMatrix();
            glTranslatef(center.x, center.y, 0.0f);
            glColor3f(color1.x, color1.y, color1.z);
            glRotatef(progression*move_amount, 0.0f, 0.0f, 1.0f);
            draw_snowflake(zero2, inner_size, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list, local_list);
        glPopMatrix();
    }
}

#endif

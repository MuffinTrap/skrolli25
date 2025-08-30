#ifndef ROTATION_FX_C
#define ROTATION_FX_C

#include <opengl_include.h>
#include "rotation_fx.h"
#include "../Ziz/screenprint.h"
#include "koch_flake.h"
//static float progression1 = 0.0f;
//static float progression2 = 0.0f;


void rotation_fx(struct Mesh* flake,
                 float radius_scale,
                 float speed,
                 float progress_normalized,
                 struct Gradient* gradient,
                 float color_1_stop,
                 float color_2_stop)
{
    progress_normalized = progress_normalized - floor(progress_normalized);
    screenprintf("Prog N %.1f\n", progress_normalized);

    float2 outerCenters[6];

    float move_amount = (360)/12.0f;
    float2 zero2 = {0.0f, 0.0f};
    if (progress_normalized <= 0.5f)
    {
        float size = 41.0f * radius_scale;
        //short recursion = fx_recursion-1;
        // Quadratic
        float pn = progress_normalized * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);

        // Extra background flake
        Gradient_glColor(gradient, color_1_stop);
            glPushMatrix();
                //screenprintf("ring center %.1f %.1f\n", ringcenter.x, ringcenter.y);
                glRotatef(progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);
                glScalef(size*1.5f, size*1.5f, 1.0f);
                Mesh_DrawArrays(flake, DrawTriangles);
                //draw_snowflake(zero2, size*1.5f, recursion+1, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
        glPopMatrix();


        Gradient_glColor(gradient, color_2_stop);
        get_corners(zero2, 6, size, progression * move_amount, outerCenters);
        for (int i = 0; i < 6; i++)
        {
            float2 ringcenter = outerCenters[i];
            glPushMatrix();
                //screenprintf("ring center %.1f %.1f\n", ringcenter.x, ringcenter.y);
                glTranslatef(ringcenter.x, ringcenter.y, 0.0f);
                glRotatef(progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);

                glScalef(size*0.5f, size*0.5f, 1.0f);
                Mesh_DrawArrays(flake, DrawTriangles);
                //draw_snowflake(zero2, size/2.0f, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);

            glPopMatrix();
        }
    }
    else if (progress_normalized > 0.5f)
    {
        //short recursion = fx_recursion;
        float size = 59.0f * radius_scale;
        float inner_size = 35.0f * radius_scale;

        // Quadratic
        float pn = (progress_normalized - 0.5f) * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);
        Gradient_glColor(gradient, color_2_stop);
        glPushMatrix();
            //glTranslatef(center.x, center.y, 0.0f);

            glPushMatrix();
            glRotatef(30.0f + progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);
            glScalef(size, size, 1.0f);
            Mesh_DrawArrays(flake, DrawTriangles);
            //draw_snowflake(zero2, size, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
            glPopMatrix();

            glPushMatrix();
            //glTranslatef(center.x, center.y, 0.0f);
            Gradient_glColor(gradient, color_1_stop);
            glRotatef(progression*move_amount, 0.0f, 0.0f, 1.0f);
            glScalef(inner_size, inner_size, 1.0f);
            Mesh_DrawArrays(flake, DrawTriangles);
            //draw_snowflake(zero2, inner_size, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
        glPopMatrix();
    }
}

#endif

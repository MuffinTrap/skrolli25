#ifndef ROTATION_FX_C
#define ROTATION_FX_C

#include <opengl_include.h>
#include "rotation_fx.h"
#include "../Ziz/screenprint.h"
#include "koch_flake.h"
//static float progression1 = 0.0f;
//static float progression2 = 0.0f;


void rotation_fx(
    struct Mesh* flake3,
    struct Mesh* flake4,
                 float radius_scale,
                 float progress_normalized,
                 struct Gradient* gradient,
                 float foreground_stop,
                 float background_stop)
{
    progress_normalized = progress_normalized - floor(progress_normalized);
    screenprintf("Prog N %.1f\n", progress_normalized);

    screenprintf("Colors : %.2f, %.2f", foreground_stop, background_stop);
    float2 outerCenters[6];



    // Screen clearing cannot be trusted


    float move_amount = (360)/12.0f;
    float2 zero2 = {0.0f, 0.0f};
    if (progress_normalized <= 0.5f)
    {
        glPushMatrix();
            glBegin(GL_QUADS);
                glColor3f(1.0f, 0.0, 0.0f); // RED
                Gradient_glColor(gradient, background_stop);
                glVertex2f(-400.0f, -400.0f);
                glVertex2f(400.0f, -400.0f);
                glVertex2f(400.0f, 400.0f);
                glVertex2f(-480.0f, 480.0f);
            glEnd();
        glPopMatrix();
        float size = 41.0f * radius_scale;
        //short recursion = fx_recursion-1;
        // Quadratic
        float pn = progress_normalized * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);

        glColor3f(0.0f, 0.0, 1.0f); // BLUE
        Gradient_glColor(gradient, foreground_stop);

        get_corners(zero2, 6, size/2, progression * move_amount, outerCenters);
        for (int i = 0; i < 6; i++)
        {
            float2 ringcenter = outerCenters[i];
            glPushMatrix();
                glTranslatef(ringcenter.x, ringcenter.y, 0.0f);
                glRotatef(progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);

                glScalef(size*0.5f, size*0.5f, 1.0f);
                Mesh_DrawArrays(flake4, DrawTriangles);

            glPopMatrix();
        }
    }
    else if (progress_normalized > 0.5f)
    {
        glPushMatrix();
            glBegin(GL_QUADS);
                glColor3f(1.0f, 0.0, 0.0f); // RED
                Gradient_glColor(gradient, background_stop);
                glVertex2f(-400.0f, -400.0f);
                glVertex2f(400.0f, -400.0f);
                glVertex2f(400.0f, 400.0f);
                glVertex2f(-480.0f, 480.0f);
            glEnd();
        glPopMatrix();
        //short recursion = fx_recursion;
        float size = 59.0f * radius_scale;
        float inner_size = 35.0f * radius_scale;

        // Quadratic
        float pn = (progress_normalized - 0.5f) * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);
        glPushMatrix();
            //glTranslatef(center.x, center.y, 0.0f);

            // Big flake behind
            glPushMatrix();

            glColor3f(0.0f, 0.0, 1.0f); // BLUE
            Gradient_glColor(gradient, foreground_stop);
                glRotatef(30.0f + progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);
                glScalef(size, size, 1.0f);
                Mesh_DrawArrays(flake4, DrawTriangles);
                //draw_snowflake(zero2, size, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
            glPopMatrix();

            // Smol in front

            glColor3f(1.0f, 0.0, 0.0f); // RED
            Gradient_glColor(gradient, background_stop);
            glPushMatrix();
                //glTranslatef(center.x, center.y, 0.0f);
                glRotatef(progression*move_amount, 0.0f, 0.0f, 1.0f);
                glScalef(inner_size, inner_size, 1.0f);
                Mesh_DrawArrays(flake4, DrawTriangles);
                //draw_snowflake(zero2, inner_size, recursion, 60.0f, 1.0f/3.0f, 1.0f, recursive_list);
            glPopMatrix();
        glPopMatrix();
    }
}

#endif

#ifndef ROTATION_FX_C
#define ROTATION_FX_C

#include <ctoy.h>
#include <opengl_include.h>
#include "rotation_fx.h"
#include "../Ziz/screenprint.h"
#include "koch_flake.h"
//static float progression1 = 0.0f;
//static float progression2 = 0.0f;

// When this is called, glTranslatef to center has already been done
void rotation_fx(
    struct Mesh* flake4,
                 float radius_scale,
                 float progress_normalized,
                 color3 fore,
                 color3 back)
{
    progress_normalized = progress_normalized - floor(progress_normalized);
    screenprintf("Prog N %.1f\n", progress_normalized);

    float2 outerCenters[6];

    screenprintf("Fore : (%.2f, %.2f %.2f)", fore.r, fore.g, fore.b);
    screenprintf("Back : (%.2f, %.2f %.2f)", back.r, back.g, back.b);

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

        get_corners(zero2, 6, size/2, progression * move_amount, outerCenters);
        for (int i = 0; i < 6; i++)
        {
            float2 ringcenter = outerCenters[i];
            glPushMatrix();
                glTranslatef(ringcenter.x, ringcenter.y, 0.0f);
                glRotatef(progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);

                glScalef(size*0.5f, size*0.5f, 1.0f);
                glColor3f(fore.r, fore.g, fore.b);
                Mesh_Draw(flake4, DrawTriangles);
                //glColor3f(1.0f, 1.0f, 1.0f);

            glPopMatrix();
        }
    }
    else
    {
        //short recursion = fx_recursion;
        float size = 59.0f * radius_scale;
        float inner_size = 35.0f * radius_scale;

        // Quadratic
        float pn = (progress_normalized - 0.5f) * 2.0f;
        float progression = 1.0f - (1.0f - pn) * (1.0f - pn);
        screenprintf("PG %.1f\n", progression);
        glPushMatrix();

            // Big flake behind
            glPushMatrix();
                glColor3f(fore.r, fore.g, fore.b);
                glRotatef(30.0f + progression * move_amount * -1.0f, 0.0f, 0.0f, 1.0f);
                glScalef(size, size, 1.0f);
                Mesh_Draw(flake4, DrawTriangles);
            glPopMatrix();

            // Smol in front

            //glColor3f(1.0f, 1.0f, 1.0f);

            glPushMatrix();
                glColor3f(back.r, back.g, back.b);
                glRotatef(progression*move_amount, 0.0f, 0.0f, 1.0f);
                glScalef(inner_size, inner_size, 1.0f);
                Mesh_Draw(flake4, DrawTriangles);
            glPopMatrix();
            //glColor3f(1.0f, 1.0f, 1.0f);
        glPopMatrix();
    }
}

#endif

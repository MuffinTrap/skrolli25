#include "gradient_mesh.h"

static void DrawVertical(struct GradientMesh* mesh)
{

    struct Gradient* g = mesh->gradient;
    float step = 1.0f / (float)g->color_amount;
    float dx = 0.0f;
    float dy = 0.0f;
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mesh->gl_texture_name);

    glBegin(GL_TRIANGLE_STRIP);

    // First 2
    Gradient_glColor(mesh->gradient, 0.0f);
    glTexCoord2f(dx, dy);
    glVertex2f(dx, dy);

    Gradient_glColor(mesh->gradient, 0.0f);
    glTexCoord2f(dx, dy);
    glVertex2f(dx + 1.0f, dy);

    for (int i = 1; i < mesh->gradient->color_amount; i++)
    {
        Gradient_glColor(mesh->gradient, dy + step);
        glTexCoord2f(dx + 1.0f, dy + step);
        glVertex2f(dx + 1.0f, dy + step);

        Gradient_glColor(mesh->gradient, dy + step);
        glTexCoord2f(dx, dy + step);
        glVertex2f(dx, dy + step);

        dy += step;
    }

    // Last 2 : if only 2 colors in gradient, loop is skipped
    Gradient_glColor(mesh->gradient, 1.0f);
    glTexCoord2f(dx, 1.0f);
    glVertex2f(dx, 1.0f);

    Gradient_glColor(mesh->gradient, 1.0f);
    glTexCoord2f(dx + 1.0f, 1.0f);
    glVertex2f(dx + 1.0f, 1.0f);

    glEnd();
    glDisable(GL_TEXTURE_2D);
}

struct GradientMesh GradientMesh_Create(struct Gradient* gradient, GLuint gl_texture_name)
{
    struct GradientMesh mesh;
    float2 zz = {0.0f, 0.0f};
    float2 oz = {1.0f, 0.0f};
    float2 oo = {1.0f, 1.0f};
    float2 zo = {0.0f, 1.0f};

    mesh.uvs[0] = zz;
    mesh.uvs[1] = oz;
    mesh.uvs[2] = oo;
    mesh.uvs[3] = zo;
    mesh.shape = GradientVertical;
    mesh.gl_texture_name = gl_texture_name;
    mesh.gradient = gradient;

    return mesh;
}

void GradientMesh_Draw(struct GradientMesh* mesh)
{
    switch(mesh->shape)
    {
        case GradientVertical: DrawVertical(mesh); break;
    }
}

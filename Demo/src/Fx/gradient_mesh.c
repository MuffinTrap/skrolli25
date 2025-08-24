#include "gradient_mesh.h"
#include <texture.h>

static void DrawVertical(struct GradientMesh* mesh)
{
    struct Gradient* g = mesh->gradient;
    float dx = 0.0f;
    float dy = 0.0f;

    float du = 0.0f;
    float dv = 1.0f;

    float ga = 1.0f;

    glBegin(GL_TRIANGLE_STRIP);

    // First 2 : left | right
    Gradient_glColorA(mesh->gradient, 0.0f, ga);
    glTexCoord2f(du, dv);
    glVertex2f(dx, dy);

    glTexCoord2f(du+1.0f, dv);
    glVertex2f(dx + 1.0f, dy);


    // TODO use the gradient stops instead of fixed step
    float step = 1.0f / (float)(g->color_amount-1);
    for (int i = 1; i < mesh->gradient->color_amount; i++)
    {
        Gradient_glColorA(mesh->gradient, dy + step, ga);
        glTexCoord2f(du, dv - step);
        glVertex2f(dx, dy + step);

        glTexCoord2f(du + 1.0f, dv - step);
        glVertex2f(dx + 1.0f, dy + step);

        dy += step;
        dv -= step;
    }

    // Last 2 : if only 2 colors in gradient, loop is skipped
    Gradient_glColorA(mesh->gradient, 1.0f, ga);
    glTexCoord2f(du, dv);
    glVertex2f(dx, dy);

    glTexCoord2f(du + 1.0f, dv);
    glVertex2f(dx + 1.0f, dy);

    glEnd();
}

static void DrawRadial(struct GradientMesh* mesh)
{
  float2 point = {1.0f, 0.0f};
  float2 rotated;
  short colors = mesh->gradient->color_amount;
  float gradient_angle = 0.0f;
  float gradient_angle_step = 1.0f / (float)colors;
  float rotation_step = M_TAU / (float)colors;
  float rotation_rad = 0.0f;
  glBegin(GL_QUADS);

  for (short i = 0; i <= colors; i++)
  {
    // loop around for last segment

    // Center and left
    Gradient_glColor(mesh->gradient, gradient_angle);
    glVertex2f(0.0f, 0.0f);

    rotated.x = cos(rotation_rad);
    rotated.y = sin(rotation_rad);
    glVertex2f(rotated.x, rotated.y);

    // right
    float next_gradient = gradient_angle + gradient_angle_step;
    if (next_gradient > 1.0f)
    {
      next_gradient -= 1.0f;
    }
    Gradient_glColor(mesh->gradient, next_gradient);
    glVertex2f(0.0f, 0.0f);

    float next_rad = rotation_rad + rotation_step;
    if (next_rad > M_TAU)
    {
      next_rad -= M_TAU;
    }
    rotated.x = cos(next_rad);
    rotated.y = sin(next_rad);
    glVertex2f(rotated.x, rotated.y);

    // Next triangle
    rotation_rad += rotation_step;
    gradient_angle += gradient_angle_step;

  }
  glEnd();

}

struct GradientMesh GradientMesh_Create(struct Gradient* gradient, GLuint gl_texture_name, enum GradientShape shape)
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
    mesh.shape = shape;
    mesh.alphamode = GradientMultiply;
    mesh.gl_texture_name = gl_texture_name;
    mesh.gradient = gradient;

    return mesh;
}

static void DrawWhite(struct GradientMesh* mesh)
{
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mesh->gl_texture_name);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
		// Lower left
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(0.0f, 0.0f);
		// Lower right
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(1.0f, 0.0f);
		// Upper right
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(1.0f, 1.0f);
		// Upper left
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(0.0f, 1.0f);
    glEnd();
    glDisable(GL_TEXTURE_2D);

}



void GradientMesh_Draw(struct GradientMesh* mesh)
{
    // TODO Draw the image first
    // and draw the gradient on top
        // Draw gradient with
        // NOTE this works but transparent is black
        // glBlendFunc(GL_ZERO, GL_SRC_COLOR);
    if (mesh->alphamode == GradientMultiply)
    {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL, 0.5f);
    }



    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, mesh->gl_texture_name);
    switch(mesh->shape)
    {
        case GradientVertical: DrawVertical(mesh); break;
        case GradientRadial: DrawRadial(mesh); break;
    }

    if (mesh->alphamode == GradientMultiply)
    {
		glDisable(GL_BLEND);
    }
    else
    {
		glDisable(GL_ALPHA_TEST);
    }
    glDisable(GL_TEXTURE_2D);
}

#include "gradient_texture.h"
#include <m_float2_math.h>
#include <texture.h>

static void DrawVerticalGradient(struct Gradient* gradient, float texture_size, bool uvs)
{
  // When drawn in the background, fill whole screen
    static const float screenWidth = 640.0f;
    float dx = -screenWidth/2;
    float dy = -screenWidth/2;
    float size = screenWidth;
    if (uvs)
    {
      dx = -0.5f * texture_size;
      dy = -0.5f * texture_size;
      size = 1.0f * texture_size;
    }

    float du = 0.0f;
    float dv = 1.0f;

    float ga = 1.0f;

    glBegin(GL_TRIANGLE_STRIP);

    // First 2 : left | right
    Gradient_glColorA(gradient, 0.0f, ga);
    if (uvs) {glTexCoord2f(du, dv);}
    glVertex2f(dx, dy);

    if (uvs) {glTexCoord2f(du+1.0f, dv);}
    glVertex2f(dx + size, dy);


    // TODO use the gradient stops instead of fixed step
    float gradient_p = 0.0f;
    float step = 1.0f / (float)(gradient->color_amount-1);
    for (int i = 1; i < gradient->color_amount; i++)
    {
        Gradient_glColorA(gradient, gradient_p + step, ga);
        if (uvs)glTexCoord2f(du, dv - step);
        glVertex2f(dx, dy + size * step);

        if (uvs)glTexCoord2f(du + 1.0f, dv - step);
        glVertex2f(dx + size, dy + step * size);

        dy += step * size;
        dv -= step;
        gradient_p += step;
    }

    // Last 2 : if only 2 colors in gradient, loop is skipped
    Gradient_glColorA(gradient, 1.0f, ga);
    if(uvs)glTexCoord2f(du, dv);
    glVertex2f(dx, dy);

    if(uvs)glTexCoord2f(du + 1.0f, dv);
    glVertex2f(dx + size, dy);

    glEnd();
}

static void DrawRadialGradient(struct Gradient* gradient, float texture_size, bool uvs)
{
  float2 point = {240.0f, 0.0f};
  float2 uvpoint = {1.0f, 0.0f};
  float2 rotated;
  float uv_inset;
  if (uvs)
  {
    point.x = texture_size;
  }
  short colors = gradient->color_amount;
  float gradient_angle = 0.0f;
  float gradient_angle_step = 1.0f / (float)colors;
  float rotation_step = M_TAU / (float)colors;
  float rotation_rad = 0.0f;
  glBegin(GL_QUADS);

  for (short i = 0; i <= colors; i++)
  {
    // loop around for last segment

    // Center
    Gradient_glColor(gradient, gradient_angle);
    if (uvs) glTexCoord2f(0.5f, 0.5f);
    glVertex2f(0.0f, 0.0f);

    // Left
    // color stays
    if (uvs)
    {
      rotated = M_ROTATE2(uvpoint, rotation_rad);
      glTexCoord2f(0.5f + rotated.x/2.0f, 0.5f + rotated.y/2.0f);
    }

    rotated = M_ROTATE2(point, rotation_rad);
    glVertex2f(rotated.x, rotated.y);

    // right
    float next_gradient = gradient_angle + gradient_angle_step;
    if (next_gradient > 1.0f)
    {
      next_gradient -= 1.0f;
    }
    Gradient_glColor(gradient, next_gradient);

    float next_rad = rotation_rad + rotation_step;
    if (next_rad > M_TAU)
    {
      next_rad -= M_TAU;
    }
    if (uvs)
    {
      rotated = M_ROTATE2(uvpoint, next_rad);
      glTexCoord2f(0.5f + rotated.x/2.0f, 0.5f + rotated.y/2.0f);
    }


    rotated = M_ROTATE2(point, next_rad);
    glVertex2f(rotated.x, rotated.y);

    // Center again
    // Color stays
    if (uvs) glTexCoord2f(0.5f, 0.5f);
    glVertex2f(0.0f, 0.0f);


    // Next triangle
    rotation_rad += rotation_step;
    gradient_angle += gradient_angle_step;

  }
  glEnd();

}

static void DrawCircleGradient(struct Gradient* gradient, float gradient_size)
{
  float points = 32.0f;
  float2 direction = {1.0f, 0.0f};
  float2 rotated_dir_left;
  float2 rotated_dir_right;
  short colors = gradient->color_amount;
  float gradient_point = 0.0f;
  float gradient_step = 1.0f / (float)colors;
  float rotation_step = M_TAU / points;
  float rotation_rad = 0.0f;
  float ring_radius = gradient_size / (float)colors;
  glBegin(GL_QUADS);

  for (short i = 0; i < points; i++)
  {
    // loop around for last segment
    float ring_left_x = 0.0f;
    float ring_left_y = 0.0f;
    float ring_right_x = 0.0f;
    float ring_right_y = 0.0f;
    rotated_dir_left = M_ROTATE2(direction, rotation_rad);
    float next_rad = rotation_rad + rotation_step;
    if (next_rad > M_TAU)
    {
      next_rad -= M_TAU;
    }
    rotated_dir_right = M_ROTATE2(direction, next_rad);

    gradient_point = 0.0f;
    for (short ring = 0; ring < colors; ring++)
    {
      // inner right
      Gradient_glColor(gradient, gradient_point);
      glVertex2f(ring_right_x, ring_right_y);
      // inner Left
      glVertex2f(ring_left_x, ring_left_y);

      // Outer left
      float next_gradient = gradient_point + gradient_step;
      if (next_gradient > 1.0f)
      {
        next_gradient -= 1.0f;
      }
      Gradient_glColor(gradient, next_gradient);

      short next_ring = ring + 1;
      ring_left_x = rotated_dir_left.x * ring_radius * next_ring;
      ring_left_y = rotated_dir_left.y * ring_radius * next_ring;
      glVertex2f(ring_left_x, ring_left_y);

      ring_right_x = rotated_dir_right.x * ring_radius * next_ring;
      ring_right_y = rotated_dir_right.y * ring_radius * next_ring;
      glVertex2f(ring_right_x, ring_right_y);

      gradient_point = next_gradient;
    }
    rotation_rad = next_rad;
  }
  glEnd();
}

struct GradientTexture GradientTexture_Create(GLuint gl_texture_name, int ziz_texture_id, enum GradientAlphaMode alphamode )
{
    struct GradientTexture texture;
    texture.alphamode = alphamode;
    texture.ziz_texture_id = ziz_texture_id;
    texture.gl_texture_name = gl_texture_name;
    texture.aspect_ratio = (float)get_texture_width(ziz_texture_id) / (float)get_texture_height(ziz_texture_id);
    return texture;
}

void GradientTexture_DrawTexture(struct GradientTexture* texture, float scale)
{
    if (texture->alphamode == GradientMultiply)
    {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
    {
		glEnable(GL_ALPHA_TEST);
		glAlphaFunc(GL_GEQUAL, 0.5f);
    }

    float ha = texture->aspect_ratio/2.0f * scale;
    float hh = scale/2.0f;
    glEnable(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture->gl_texture_name);

    glColor3f(1.0f, 1.0f, 1.0f);
    glBegin(GL_QUADS);
		// Lower left
		glTexCoord2f(0.0f, 1.0f);
		glVertex2f(-ha, -hh);
		// Lower right
		glTexCoord2f(1.0f, 1.0f);
		glVertex2f(ha, -hh);
		// Upper right
		glTexCoord2f(1.0f, 0.0f);
		glVertex2f(ha, hh);
		// Upper left
		glTexCoord2f(0.0f, 0.0f);
		glVertex2f(-ha, hh);
    glEnd();
    glDisable(GL_TEXTURE_2D);

    if (texture->alphamode == GradientMultiply)
    {
		glDisable(GL_BLEND);
    }
    else
    {
		glDisable(GL_ALPHA_TEST);
    }
}

void GradientTexture_Draw(struct GradientTexture* texture, struct Gradient* gradient, float texture_size, float gradient_size)
{
    // TODO Draw the image first
    // and draw the gradient on top
        // Draw gradient with
        // NOTE this works but transparent is black
        /*
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
    */
    switch (texture->alphamode)
    {
      case GradientMultiply:
      {
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

        glEnable(GL_TEXTURE_2D);
        glBindTexture(GL_TEXTURE_2D, texture->gl_texture_name);

        DrawVerticalGradient(gradient, texture_size, true); break;
        glDisable(GL_TEXTURE_2D);
        glDisable(GL_BLEND);
      }
      break;
      case GradientCutout:
      {
        switch(gradient->shape)
        {
          case GradientVertical: DrawVerticalGradient(gradient, gradient_size, false); break;
          case GradientRadial: DrawRadialGradient(gradient, gradient_size, false); break;
          case GradientCircle: DrawCircleGradient(gradient, gradient_size ); break;
        }
        GradientTexture_DrawTexture(texture, texture_size);

      }
      break;
    }
}

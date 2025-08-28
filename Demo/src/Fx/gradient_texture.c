#include "gradient_texture.h"
#include "../Ziz/screenprint.h"
#include <m_float2_math.h>
#include <texture.h>

static void DrawVerticalGradient(struct Gradient* gradient, float texture_size, bool uvs, float gradient_offset, float gradient_repeat)
{
  float smoothness = 10.0f;
  //screenprintf("Gradient repeat %.2f", gradient_repeat);
  // When drawn in the background, fill whole screen
    static const float screenWidth = 640.0f;
    static const float screenHeight = 480.0f;
    float dx = -screenWidth/2;
    float dy = -screenHeight/2;
    float size_x = screenWidth;
    float size_y = screenHeight;
    if (uvs)
    {
      dx = -0.5f * texture_size;
      dy = -0.5f * texture_size;
      size_x = 1.0f * texture_size;
      size_y = 1.0f * texture_size;
    }

    float du = 0.0f;
    float dv = 1.0f;

    float ga = gradient->alpha;

    glBegin(GL_TRIANGLE_STRIP);
    //glBegin(GL_LINE_LOOP);

    float gradient_p = gradient_offset;
    // First 2 : left | right
    //screenprintf("Gradient point %d: %.2f", 0, gradient_p);
    Gradient_glColorA(gradient, gradient_p, ga);
    if (uvs) {glTexCoord2f(du, dv);}
    glVertex2f(dx, dy);

    if (uvs) {glTexCoord2f(du+1.0f, dv);}
    glVertex2f(dx + size_x, dy);


    // TODO use the gradient stops instead of fixed step

    int slices = gradient_repeat * (gradient->color_amount-1) * smoothness;
    //screenprintf("Slices %d", slices);

    float gradient_step = (1.0f / (float)(gradient->color_amount-1) / smoothness);
    //screenprintf("Gradient step %.2f", gradient_step);

    float y_step = size_y / ((float)slices);
    float v_step = 1.0f / ((float)slices);
    for (int i = 1; i < slices; i++)
    {
        Gradient_glColorA(gradient, gradient_p + gradient_step, ga);
        if (uvs)glTexCoord2f(du, dv - v_step);
        glVertex2f(dx, dy + y_step);

        if (uvs)glTexCoord2f(du + 1.0f, dv - v_step);
        glVertex2f(dx + size_x, dy + y_step);

        dy += y_step;
        dv -= v_step;
        gradient_p += gradient_step;
     //   screenprintf("Gradient point %d: %.2f", i, gradient_p);
    }

    dy += y_step;
    dv -= v_step;
    gradient_p += gradient_step;

    // Last 2 : if only 2 colors in gradient, loop is skipped
    //screenprintf("Gradient point %d: %.2f", slices, gradient_p);
    Gradient_glColorA(gradient, gradient_p, ga);
    if(uvs)glTexCoord2f(du, dv);
    glVertex2f(dx, dy);

    if(uvs)glTexCoord2f(du + 1.0f, dv);
    glVertex2f(dx + size_x, dy);

    glEnd();
}

static void DrawRadialGradient(struct Gradient* gradient, float texture_size, float gradient_size, bool uvs, float gradient_offset, float gradient_repeat)
{
  float2 point = {gradient_size, 0.0f};
  float2 uvpoint = {1.0f, 0.0f};
  float2 rotated;
  float uv_inset;
  if (uvs)
  {
    point.x = texture_size;
  }
  short colors = gradient->color_amount;
  short corners = colors * gradient_repeat;
  float gradient_angle = gradient_offset * M_TAU;
  float gradient_angle_step = 1.0f / ((float)colors);
  float rotation_step = M_TAU / (float)corners;
  float rotation_rad = 0.0f;
  glBegin(GL_QUADS);

  for (short i = 0; i <= corners; i++)
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

static void DrawCircleGradient(struct Gradient* gradient, float gradient_size, float gradient_offset, float gradient_repeat)
{
  float smoothness = 10.0f;
  float points = 32.0f;
  float2 direction = {1.0f, 0.0f};
  float2 rotated_dir_left;
  float2 rotated_dir_right;
  short colors = gradient->color_amount;
  float gradient_point = gradient_offset;
  float gradient_step = 1.0f / ((float)colors*smoothness);
  float rotation_step = M_TAU / points;
  float rotation_rad = 0.0f;
  short rings = colors * gradient_repeat * smoothness;
  float ring_radius = gradient_size / rings;
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

    gradient_point = gradient_offset;
    for (short ring = 0; ring < rings; ring++)
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

void GradientTexture_Draw(struct GradientTexture* texture, struct Gradient* gradient, float texture_size, float gradient_size, float gradient_offset, float gradient_repeat)
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
        GradientTexture_DrawTexture(texture, texture_size);

		glEnable(GL_BLEND);
		glBlendFunc(GL_DST_COLOR, GL_ZERO);

        switch(gradient->shape)
        {
          case GradientVertical: DrawVerticalGradient(gradient, texture_size, false, gradient_offset, gradient_repeat); break;
          case GradientRadial: DrawRadialGradient(gradient, 0.0f, gradient_size, false, gradient_offset, gradient_repeat); break;
          case GradientCircle: DrawCircleGradient(gradient, gradient_size, gradient_offset, gradient_repeat ); break;
        }
        glDisable(GL_BLEND);
      }
      break;
      case GradientCutout:
      {
        switch(gradient->shape)
        {
          case GradientVertical: DrawVerticalGradient(gradient, texture_size, false, gradient_offset, gradient_repeat); break;
          case GradientRadial: DrawRadialGradient(gradient, 0.0f, gradient_size, false, gradient_offset, gradient_repeat); break;
          case GradientCircle: DrawCircleGradient(gradient, gradient_size, gradient_offset, gradient_repeat ); break;
        }
        GradientTexture_DrawTexture(texture, texture_size);

      }
      break;
    }
}

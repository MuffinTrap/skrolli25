#ifndef GRADIENT_TEXTURE_H
#define GRADIENT_TEXTURE_H
// Uses gradient and shape
// has texture
// animated vertices and colors
#include "gradient.h"
#include <m_math.h>
#include <opengl_include.h>

enum GradientAlphaMode
{
    GradientMultiply,
    GradientCutout
};

struct GradientTexture
{
    int ziz_texture_id;
    GLuint gl_texture_name;
    enum GradientAlphaMode alphamode;
    float aspect_ratio;
};

struct GradientTexture GradientTexture_Create(GLuint gl_texture_name, int ziz_texture_id, enum GradientAlphaMode alphamode);

/**
 * @brief Draws both the texture and gradient. Uses the given AlphaMode
 */
void GradientTexture_Draw(struct GradientTexture* texture, struct Gradient* gradient, float texture_size, float gradient_size, float gradient_offset);

/**
 * @brief Draws only the texture using alphamode
 */
void GradientTexture_DrawTexture(struct GradientTexture* texture, float scale);

/**
 * @brief Draws only the gradient. Texture should be drawn before or after, depending on alphamode
 * @param gradient The gradient
 * @param alphamode The mode the texture was/will be drawn in
 * @param repeats How many times to repeat the gradient
 */
void GradientTexture_DrawGradient(struct Gradient* gradient, enum GradientAlphaMode alphamode, float size, float offset);

void GradientTexture_SetFiltering(struct GradientTexture* texture, GLenum mode);



#endif

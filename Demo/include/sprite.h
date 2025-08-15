/*========================================================================
Ziz - version 0.2
A fork of Ctoy by Anael Seghezzi <www.maratis3d.com>
Copyright (c) 2025 Sofia Savilampi
Original work Copyright (c) [year] Anael Seghezzi

This software is provided 'as-is', without any express or implied warranty.
In no event will the authors be held liable for any damages arising from its use.

Permission is granted to anyone to use this software for any purpose,
including commercial applications, and to alter it and redistribute it
freely, subject to the following restrictions:

1. The origin of this software must not be misrepresented; you must not
   claim that you wrote the original software. If you use this software
   in a product, an acknowledgment would be appreciated but is not required.
2. Altered source versions must be plainly marked as such, and must not
   be misrepresented as being the original software.
3. This notice may not be removed or altered from any source distribution.
=======================================================================*/
#ifndef SPRITE_H
#define SPRITE_H

#include "surface.h"
#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>

#ifdef __APPLE__
#include <OpenGL/gl.h>
#include <OpenGL/glu.h>
#else
#include <GL/gl.h>
#include <GL/glu.h>
#endif

typedef struct {
    surface_t *surface;    // Texture data
    int16_t width;         // Sprite width (pixels)
    int16_t height;        // Sprite height (pixels)
    int16_t hslices;       // Horizontal slices (for animations)
    int16_t vslices;       // Vertical slices (for animations)
    float scale_x;         // Horizontal scale
    float scale_y;         // Vertical scale
    bool flip_x;           // Flip horizontally
    bool flip_y;           // Flip vertically
    uint8_t alpha;         // Alpha transparency (0-255)
} sprite_t;

// Sprite creation/destruction
sprite_t *sprite_create(int width, int height, tex_format_t format);
void sprite_free(sprite_t *sprite);

// Sprite loading (from file or memory)
sprite_t *sprite_load(const char *filename);
sprite_t *sprite_load_buf(const void *data, size_t size);

// Drawing functions
void sprite_draw(const sprite_t *sprite, int x, int y);
void sprite_draw_scaled(const sprite_t *sprite, int x, int y, float scale_x, float scale_y);
void sprite_draw_slice(const sprite_t *sprite, int x, int y, int slice_x, int slice_y);
void sprite_draw_transformed(const sprite_t *sprite, int x, int y, 
                           float scale_x, float scale_y, bool flip_x, bool flip_y, uint8_t alpha);

// Animation helpers
void sprite_set_slices(sprite_t *sprite, int hslices, int vslices);
void sprite_get_slice_size(const sprite_t *sprite, int *slice_w, int *slice_h);

// OpenGL-specific helpers
GLuint sprite_get_texture(const sprite_t *sprite);

#endif // SPRITE_H
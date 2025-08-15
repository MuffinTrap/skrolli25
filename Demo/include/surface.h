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
#ifndef SURFACE_H
#define SURFACE_H

#ifdef __cplusplus
extern "C" {
#endif

// Core headers
#include <stddef.h>  // NULL, size_t
#include <stdint.h>  // uint16_t, uint32_t
#include <stdlib.h>  // malloc, free
#include <math.h>    // powf, fmaxf, fminf
#include "gl2.h"

#include "m_image_types.h"

//accepts surfaces in either FMT_RGBA16 or FMT_RGBA32 as target buffers, and does not assert. 
typedef enum {
    FMT_NONE=0, // Placeholder for no format defined.
    FMT_RGBA16, // Format RGBA 5551 (16-bit)
    FMT_RGBA32, // Format RGBA 8888 (32-bit)
    FMT_YUV16, // Format YUV2 4:2:2 (data interleaved as YUYV)
    FMT_CI4, // Format CI4: color index 4-bit (paletted, 2 indices per byte)
    FMT_CI8, // Format CI8: color index 8-bit (paletted, 1 index per byte)
    FMT_IA4, // Format IA4: 3-bit intensity + 1-bit alpha (4-bit per pixel)
    FMT_IA8, // Format IA8: 4-bit intensity + 4-bit alpha (8-bit per pixel)
    FMT_IA16, // Format IA16: 8-bit intensity + 8-bit alpha (16-bit per pixel)
    FMT_I4, // Format I4: 4-bit intensity (4-bit per pixel)
    FMT_I8 // Format I8: 8-bit intensity (8-bit per pixel) 
} tex_format_t;

typedef struct {
    tex_format_t format;
    uint16_t width;
    uint16_t height;
    uint16_t stride;
    void *buffer;
    void *userdata;  // For OpenGL texture handle
    
    // Fields ctoy expects
    int type;        // M_FLOAT or M_UBYTE
    int comp;        // Components (3=RGB, 4=RGBA)
    void *data;      // Alias for buffer
} surface_t;



//-----------------------------------------------------------------
// Core surface functions
//-----------------------------------------------------------------
inline surface_t surface_empty() {
    surface_t empty_surface =  {
        .format = FMT_RGBA32,  // Or your default format
        .width = 0,
        .height = 0,
        .stride = 0,
        .buffer = NULL,
        .userdata = NULL,
        .type = M_UBYTE,       // Or your default type
        .comp = 4,             // Typically 4 for RGBA
        .data = NULL
    };
    return empty_surface;
}

inline surface_t surface_make(void *buffer, tex_format_t format, uint16_t width, uint16_t height, uint16_t stride) {
    surface_t surface;
    surface.buffer = buffer;
    surface.format = format;
    surface.width = width;
    surface.height = height;
    surface.stride = stride ? stride : width * (format == FMT_RGBA32 ? 4 : 2); // Auto-calculate stride if 0
    return surface;
}

inline surface_t create_backbuffer(uint16_t width, uint16_t height) {
    surface_t empty_surface;
    surface_make(&empty_surface, FMT_RGBA32, width, height, 0);
    return empty_surface;
}

inline surface_t *surface_alloc(tex_format_t format, uint16_t width, uint16_t height) {
    int bpp = (format == FMT_RGBA32) ? 4 : 2; // Bytes per pixel
    surface_t *surface = malloc(sizeof(surface_t));
    surface->buffer = malloc(width * height * bpp);
    surface->format = format;
    surface->width = width;
    surface->height = height;
    surface->stride = width * bpp;
    printf("---Surface allocated, %i x %i with bit depth of %i---\n", width, height, bpp ? 32 : 16);
    return surface;
}

inline void surface_free(surface_t *surface) {
    if (surface) {
        free(surface->buffer);
        free(surface);
    }
}

//-----------------------------------------------------------------
// Pixel operations
//-----------------------------------------------------------------

inline void surface_draw_pixel(surface_t *surface, int x, int y, uint32_t color) {
    if (x < 0 || x >= surface->width || y < 0 || y >= surface->height)
        return;

    switch (surface->format) {
        case FMT_RGBA16: {
            uint16_t *pixels = (uint16_t*)surface->buffer;
            uint16_t r = (color >> 16) & 0xF0;
            uint16_t g = (color >> 8)  & 0xF0;
            uint16_t b = (color >> 0)  & 0xF0;
            uint16_t a = (color >> 24) & 0xF0;
            pixels[y * (surface->stride / 2) + x] = (a << 12) | (r << 8) | (g << 4) | b;
            break;
        }
        case FMT_RGBA32: {
            uint32_t *pixels = (uint32_t*)surface->buffer;
            pixels[y * (surface->stride / 4) + x] = color;
            break;
        }
        default:
            break; // Unsupported formats (FMT_I8, FMT_IA16)
    }
}

inline uint32_t surface_read_pixel(const surface_t *surface, int x, int y) {
    if (x < 0 || x >= surface->width || y < 0 || y >= surface->height)
        return 0;

    switch (surface->format) {
        case FMT_RGBA16: {
            uint16_t pixel = ((uint16_t*)surface->buffer)[y * (surface->stride / 2) + x];
            return ((pixel >> 11) & 0x1F) << 19 |  // R
                   ((pixel >> 5)  & 0x3F) << 10 |  // G
                   ((pixel >> 0)  & 0x1F) << 3  |  // B
                   0xFF000000;                     // Alpha
        }
        case FMT_RGBA32: {
            return ((uint32_t*)surface->buffer)[y * (surface->stride / 4) + x];
        }
        default:
            return 0;
    }
}

//-----------------------------------------------------------------
// OpenGL helpers
//-----------------------------------------------------------------

inline void surface_to_rgba32(const surface_t *surface, uint32_t *out_rgba32) {
    for (int y = 0; y < surface->height; y++) {
        for (int x = 0; x < surface->width; x++) {
            out_rgba32[y * surface->width + x] = surface_read_pixel(surface, x, y);
        }
    }
}
/**
 * Convert a floating point surface to SRGB (8-bit per channel) format
 * 
 * @param dest Destination surface (must be M_UBYTE format)
 * @param src Source surface (must be M_FLOAT format)
 */
inline void surface_float_to_srgb(surface_t *dest, const surface_t *src) {
    // Validate inputs
    if (!dest || !src || !dest->buffer || !src->buffer) return;
    if (dest->type != M_UBYTE || src->type != M_FLOAT) return;
    if (dest->width != src->width || dest->height != src->height) return;
    
    const float *src_pixels = (const float *)src->buffer;
    unsigned char *dest_pixels = (unsigned char *)dest->buffer;
    const int components = src->comp;
    const int pixel_count = src->width * src->height;
    
    for (int i = 0; i < pixel_count; i++) {
        for (int c = 0; c < components; c++) {
            // Convert from linear to sRGB
            float linear = src_pixels[i * components + c];
            float srgb;
            
            if (linear <= 0.0031308f) {
                srgb = linear * 12.92f;
            } else {
                srgb = 1.055f * powf(linear, 1.0f/2.4f) - 0.055f;
            }
            
            // Clamp and convert to 8-bit
            srgb = fmaxf(0.0f, fminf(1.0f, srgb));
            dest_pixels[i * components + c] = (unsigned char)(srgb * 255.0f);
        }
        
        // Pad with 255 for alpha if needed
        if (components < 4 && dest->comp == 4) {
            dest_pixels[i * 4 + 3] = 255; // Full alpha
        }
    }
}
inline void surface_render_opengl(const surface_t *surface) {
    // Convert to RGBA32 if needed
    uint32_t *rgba32_buffer = malloc(surface->width * surface->height * 4);
    surface_to_rgba32(surface, rgba32_buffer);

    // Upload to OpenGL
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, surface->width, surface->height, 0,
                 GL_RGBA, GL_UNSIGNED_BYTE, rgba32_buffer);

    free(rgba32_buffer);
}

#ifdef __cplusplus
}
#endif
#endif // SURFACE_H

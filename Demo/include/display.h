#ifndef DISPLAY_H
#define DISPLAY_H

#ifdef __cplusplus
extern "C" {
#endif

#include "surface.h"
#include "gl2.h"
#include <stdint.h>

// Resolution types
typedef enum {
    RESOLUTION_256x240,
    RESOLUTION_320x240,
    RESOLUTION_512x240,
    RESOLUTION_640x240, 
    RESOLUTION_512x480,
    RESOLUTION_640x480
} resolution_t;

// Bit depth options
typedef enum {
    DEPTH_16_BPP,
    DEPTH_32_BPP
} bitdepth_t;

// Gamma options
typedef enum {
    GAMMA_NONE,
    GAMMA_CORRECT,
    GAMMA_CORRECT_DITHER
} gamma_t;

// Filter options
typedef enum {
    FILTERS_DISABLED,
    FILTERS_RESAMPLE,
    FILTERS_DEDITHER,
    FILTERS_RESAMPLE_ANTIALIAS,
    FILTERS_RESAMPLE_ANTIALIAS_DEDITHER
} filter_options_t;

// Initialize display with specified settings
void display_init(resolution_t res, bitdepth_t bit, uint32_t num_buffers,
                 gamma_t gamma, filter_options_t filters);

// Get current display surface
surface_t* display_get(void);

// Swap buffers and display
void display_show(void);

// Clean up resources
void display_close(void);

int display_get_width(void);
int display_get_height(void);
inline void rdpq_attach(surface_t *disp, surface_t *depth) { /* This is fake, just simplifies N64 integration */}
void gl_context_begin();
void gl_context_end();
inline void rdpq_detach_show() { /* This is fake, just simplifies N64 integration */}
#ifdef __cplusplus
}
#endif

#endif /* DISPLAY_H */
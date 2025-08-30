#ifndef RAT_TYPES_H
#define RAT_TYPES_H

#include <animation.h>

#define MAX_NAME_LENGTH 128
// Transform structure for positioning, rotation, and scaling
typedef struct {
    float position[3];    // x, y, z position
    float rotation[3];    // x, y, z rotation (in radians)
    float scale[3];       // x, y, z scale factors
} RatTransform;

// RAT Model structure containing all necessary data
typedef struct {
    char name[MAX_NAME_LENGTH];
    RatAnimationInfo* anim_info;
    DecompressionContext* context;
    int texture_id;
    RatTransform transform;
    bool is_valid;
    bool bounds_fixed;
    uint32_t current_frame;
    uint32_t total_frames;

#ifdef N64
    // N64 Frame skipping optimization for high-vertex models
    bool frame_skipping_enabled;     // Whether frame skipping is active
    uint32_t effective_total_frames; // Number of frames actually used (may be half of total_frames)
    uint32_t frame_skip_factor;      // Skip every N frames (2 = every other frame)
#endif

#ifndef NO_DISPLAY_LISTS
    // Display list support - simplified to only first frame
    GLuint first_frame_display_list;  // Display list for frame 0 only
    bool first_frame_compiled;        // Flag indicating if first frame display list is compiled
    GLenum last_display_list_error;   // Last OpenGL error during display list operations
    bool failed_display_list;         // Flag indicating if display list generation failed
    bool force_immediate_mode;        // User-controlled flag to force immediate mode rendering
#else
    GLuint first_frame_display_list;
    bool first_frame_compiled;
    bool failed_display_list;
    bool force_immediate_mode;
#endif
} RatModel;
#endif

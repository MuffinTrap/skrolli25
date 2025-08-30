/*
 * rat_handler.c - RAT Animation File Handler
 * 
 * This module provides comprehensive handling for RAT animation files including:
 * - Loading and initialization
 * - Texture management 
 * - Transformation operations
 * - Rendering with custom positioning and scaling
 * - Named model instances for easy management
 * 
 * Usage:
 *   RatModel* model = rat_model_create("my_model", "assets/animation.rat", "assets/texture.png");
 *   rat_model_set_transform(model, pos, rot, scale);
 *   rat_model_update(model, frame_number);
 *   rat_model_render(model);
 *   rat_model_destroy(model);
 */

// Define this to enable profiler function declarations in animation.h
#define N64_PROFILER_IMPLEMENTATION


// Maximum number of named RAT models that can be managed
#define MAX_RAT_MODELS 64
#define MAX_NAME_LENGTH 64

// Texture ID constant (should match texture.c)
#define INVALID_TEXTURE_ID -1
#ifdef GEKKO
#define NO_DISPLAY_LISTS 1
#endif
// Macro to determine if display lists should be used for a model
// Only use display lists for frame 0 to save memory while still getting some performance benefit
#ifdef NO_DISPLAY_LISTS
#define USE_DISPLAY_LISTS(model) (false)
#else
#define USE_DISPLAY_LISTS(model) \
    (!(model)->force_immediate_mode && (model)->current_frame == 0 && (model)->first_frame_compiled && !(model)->failed_display_list)
#endif

#include <rat_types.h>
// Global registry of RAT models
static RatModel rat_models[MAX_RAT_MODELS];
static int num_rat_models = 0;
static bool rat_handler_initialized = false;

// --- Initialization and Cleanup ---

void rat_handler_init(void) {
    if (rat_handler_initialized) return;
    
    // Initialize all model slots
    for (int i = 0; i < MAX_RAT_MODELS; i++) {
        memset(&rat_models[i], 0, sizeof(RatModel));
        rat_models[i].is_valid = false;
    }
    
    num_rat_models = 0;
    rat_handler_initialized = true;
#ifndef N64
    printf("RAT Handler: Initialized with %d model slots\n", MAX_RAT_MODELS);
#endif
}

void rat_model_update(RatModel* model, uint32_t frame);
// Simplified display list functions (first frame only)
static void rat_model_cleanup_display_lists(RatModel* model);
static void rat_model_compile_first_frame_display_list(RatModel* model);
static void rat_model_cleanup_first_frame_display_list(RatModel* model);

void rat_handler_cleanup(void) {
    if (!rat_handler_initialized) return;
    
    // Clean up all models
    for (int i = 0; i < MAX_RAT_MODELS; i++) {
        if (rat_models[i].is_valid) {
            // Clean up display lists first
            rat_model_cleanup_first_frame_display_list(&rat_models[i]);
            
            rat_free_context(rat_models[i].context);
            rat_free_animation(rat_models[i].anim_info);
            // Note: texture cleanup would depend on your texture management system
        }
    }
    
    num_rat_models = 0;
    rat_handler_initialized = false;
#ifndef N64
    printf("RAT Handler: Cleaned up\n");
#endif
}

// --- Helper Functions ---

static int find_free_model_slot(void) {
    for (int i = 0; i < MAX_RAT_MODELS; i++) {
        if (!rat_models[i].is_valid) {
            return i;
        }
    }
    return -1; // No free slots
}

static int find_model_by_name(const char* name) {
    if (!name) return -1;
    
    for (int i = 0; i < MAX_RAT_MODELS; i++) {
        if (rat_models[i].is_valid && strcmp(rat_models[i].name, name) == 0) {
            return i;
        }
    }
    return -1; // Not found
}

static void rat_model_fix_bounds(RatModel* model) {
    if (!model || !model->anim_info || !model->context || model->bounds_fixed) return;
    
    RatAnimationInfo* anim = model->anim_info;
    DecompressionContext* ctx = model->context;
    
    // Check if model bounds are invalid (all zeros or equal)
    bool invalid_bounds = (anim->min_x == anim->max_x || 
                          anim->min_y == anim->max_y || 
                          anim->min_z == anim->max_z);
    
    if (invalid_bounds) {
#ifndef N64
        printf("RAT Handler: Fixing invalid bounds for model '%s'\n", model->name);
#endif
        
        // Set reasonable default bounds
        anim->min_x = -1.0f; anim->max_x = 1.0f;
        anim->min_y = -1.0f; anim->max_y = 1.0f;
        anim->min_z = -1.0f; anim->max_z = 1.0f;
        
        // Update context boundsF
        ctx->min_x = anim->min_x; ctx->max_x = anim->max_x;
        ctx->min_y = anim->min_y; ctx->max_y = anim->max_y;
        ctx->min_z = anim->min_z; ctx->max_z = anim->max_z;
        
        // Recalculate denormalized vertex positions
        for (uint32_t v = 0; v < ctx->num_vertices; v++) {
            float norm_x = (float)ctx->current_positions[v].x / 255.0f;
            float norm_y = (float)ctx->current_positions[v].y / 255.0f;
            float norm_z = (float)ctx->current_positions[v].z / 255.0f;
            
            ctx->current_frame_vertices[v*3 + 0] = ctx->min_x + norm_x * (ctx->max_x - ctx->min_x);
            ctx->current_frame_vertices[v*3 + 1] = ctx->min_y + norm_y * (ctx->max_y - ctx->min_y);
            ctx->current_frame_vertices[v*3 + 2] = ctx->min_z + norm_z * (ctx->max_z - ctx->min_z);
        }
    }
    
    model->bounds_fixed = true;
}

#ifdef N64
// N64 Frame skipping optimization - skip frames for high-vertex models to save memory
static void rat_model_configure_n64_frame_skipping(RatModel* model) {
    if (!model || !model->context) return;
    
    uint32_t vertex_count = model->context->num_vertices;
    uint32_t total_frames = model->anim_info->num_frames;
    
    // Enable frame skipping for models with >120 vertices to save memory on N64
    if (vertex_count > 120 && total_frames > 8) {
        model->frame_skipping_enabled = true;
        model->frame_skip_factor = 2; // Skip every x frame
        model->effective_total_frames = (total_frames + 1) / model->frame_skip_factor; // Round up division

        printf("N64: Enabling frame skipping for '%s' (%u vertices, %u->%u frames)\n", 
               model->name, vertex_count, total_frames, model->effective_total_frames);
    } else {
        model->frame_skipping_enabled = false;
        model->frame_skip_factor = 1;
        model->effective_total_frames = total_frames;
        
        if (vertex_count > 120) {
            printf("N64: Not skipping frames for '%s' (too few frames: %u)\n", 
                   model->name, total_frames);
        }
    }
}

// Convert logical frame to actual frame index (handles frame skipping)
static uint32_t rat_model_logical_to_actual_frame(RatModel* model, uint32_t logical_frame) {
    if (!model->frame_skipping_enabled) {
        return logical_frame % model->total_frames;
    }
    
    // Map logical frame to actual frame with skipping
    uint32_t actual_frame = (logical_frame * model->frame_skip_factor) % model->total_frames;
    return actual_frame;
}

// Convert actual frame to logical frame (for display/UI purposes)
static uint32_t rat_model_actual_to_logical_frame(RatModel* model, uint32_t actual_frame) {
    if (!model->frame_skipping_enabled) {
        return actual_frame;
    }
    
    return actual_frame / model->frame_skip_factor;
}
#endif

#ifndef NO_DISPLAY_LISTS
// --- Display List Management ---

static void rat_model_compile_display_lists(RatModel* model) {
    if (!model || !model->is_valid || !model->context || !model->anim_info) {
        return;
    }
    
    // Clean up existing display list first
    rat_model_cleanup_first_frame_display_list(model);
    
    // Compile only the first frame (frame 0) into a display list
    rat_model_compile_first_frame_display_list(model);
}

static void rat_model_cleanup_display_lists(RatModel* model) {
    // Just redirect to the first frame cleanup function
    rat_model_cleanup_first_frame_display_list(model);
}

#endif

#ifndef NO_DISPLAY_LISTS
static bool rat_model_render_display_list(RatModel* model, uint32_t frame) {
    // Only render with display list if it's frame 0 and the display list is compiled
    if (!model || !model->is_valid || frame != 0 || !model->first_frame_compiled) {
        return false;
    }
    
    // Use safe display list rendering for frame 0
    rat_render_display_list_safe(model->first_frame_display_list);
    
    return true;
}
#else
// NO_DISPLAY_LISTS is defined (GEKKO platform) - provide stub function
static bool rat_model_render_display_list(RatModel* model, uint32_t frame) {
    // Always return false on GEKKO - never use display lists
    return false;
}
#endif

// --- First Frame Display List Management (available on non-GEKKO platforms only) ---

#ifndef NO_DISPLAY_LISTS
static void rat_model_compile_first_frame_display_list(RatModel* model) {
    if (!model || !model->is_valid || !model->context) {
        return;
    }

#ifdef N64
    // On N64, GL context might not be ready at creation time.
    // Defer compilation until the first render call.
    extern void* state;
    if (state == NULL) {
        return; // Will try again in render loop
    }
#endif

    model->first_frame_display_list = glGenLists(1);
    if (model->first_frame_display_list == 0) {
        // Failed to generate
        model->first_frame_compiled = false;
        model->failed_display_list = true;
        return;
    }

    // Decompress to frame 0 if not already there
    uint32_t original_frame = model->current_frame;
    if (original_frame != 0) {
        rat_model_update(model, 0);
    }

    glNewList(model->first_frame_display_list, GL_COMPILE);
    rat_render_opengl(model->context, GL_TRIANGLES);
    glEndList();

    // With per-context state tracking, no need to reset any global state
    // Each context maintains its own state independently

    // Restore original frame
    if (original_frame != 0) {
        rat_model_update(model, original_frame);
    }

    model->first_frame_compiled = true;
    model->failed_display_list = false;
}

static void rat_model_cleanup_first_frame_display_list(RatModel* model) {
    if (model && model->first_frame_compiled && model->first_frame_display_list != 0) {
        glDeleteLists(model->first_frame_display_list, 1);
        model->first_frame_display_list = 0;
        model->first_frame_compiled = false;
    }
}

#else
// NO_DISPLAY_LISTS is defined (GEKKO platform) - provide stub functions
static void rat_model_compile_first_frame_display_list(RatModel* model) {
    // No-op on GEKKO
    if (model) {
        model->first_frame_compiled = false;
        model->failed_display_list = false;
    }
}

static void rat_model_cleanup_first_frame_display_list(RatModel* model) {
    // No-op on GEKKO
    if (model) {
        model->first_frame_compiled = false;
    }
}
#endif

// --- Public API Functions ---

RatModel* rat_model_create(const char* name, const char* rat_file_path, const char* texture_path) {
    if (!rat_handler_initialized) {
        rat_handler_init();
    }
    
    if (!name || !rat_file_path) {
#ifndef N64
        printf("RAT Handler: Invalid parameters for model creation\n");
#endif
        return NULL;
    }
    
    // Check if name already exists
    if (find_model_by_name(name) >= 0) {
#ifndef N64
        printf("RAT Handler: Model with name '%s' already exists\n", name);
#endif
        return NULL;
    }
    
    // Find free slot
    int slot = find_free_model_slot();
    if (slot < 0) {
#ifndef N64
        printf("RAT Handler: No free slots available (max %d models)\n", MAX_RAT_MODELS);
#endif
        return NULL;
    }
    
    RatModel* model = &rat_models[slot];
    
    // Initialize the model
    strncpy(model->name, name, MAX_NAME_LENGTH - 1);
    model->name[MAX_NAME_LENGTH - 1] = '\0';
    
    // Load RAT dec
    model->anim_info = rat_read_file(rat_file_path);
    if (!model->anim_info) {
        const char* error = rat_get_last_error();
#ifndef N64
        printf("RAT Handler: Failed to load RAT file '%s': %s\n", rat_file_path, error);
#endif
        return NULL;
    }
    
    // Create decompression context
    model->context = rat_create_context(model->anim_info);
    if (!model->context) {
        const char* error = rat_get_last_error();
#ifndef N64
        printf("RAT Handler: Failed to create context for '%s': %s\n", name, error);
#endif
        rat_free_animation(model->anim_info);
        return NULL;
    }
    
    // Load texture if provided
    model->texture_id = INVALID_TEXTURE_ID;  // Initialize to invalid
    if (texture_path) {
        model->texture_id = addTexture(texture_path);
        if (model->texture_id == INVALID_TEXTURE_ID) {
#ifndef N64
            printf("RAT Handler: Warning - Failed to load texture '%s' for model '%s'\n", 
                   texture_path, name);
#endif
        } else {
#ifndef N64
            printf("RAT Handler: Successfully loaded texture '%s' for model '%s' (Manager ID: %d)\n", 
                   texture_path, name, model->texture_id);
#endif
        }
    }
    
    // Initialize transform to defaults
    model->transform.position[0] = 0.0f;
    model->transform.position[1] = 0.0f;
    model->transform.position[2] = 0.0f;
    model->transform.rotation[0] = 0.0f;
    model->transform.rotation[1] = 0.0f;
    model->transform.rotation[2] = 0.0f;
    model->transform.scale[0] = 1.0f;
    model->transform.scale[1] = 1.0f;
    model->transform.scale[2] = 1.0f;
    
    model->current_frame = 0;
    model->total_frames = model->anim_info->num_frames;
    model->is_valid = true;
    model->bounds_fixed = false;
    
#ifdef N64
    // Initialize N64 frame skipping fields (will be configured in compile_display_lists)
    model->frame_skipping_enabled = false;
    model->effective_total_frames = model->total_frames;
    model->frame_skip_factor = 1;
#endif
    
    // Initialize simplified display list fields (available on all platforms now)
    model->first_frame_display_list = 0;
    model->first_frame_compiled = false;
    model->failed_display_list = false;
    model->force_immediate_mode = false;
    
#ifndef NO_DISPLAY_LISTS
    model->last_display_list_error = GL_NO_ERROR;
#endif
    
    // Fix bounds if necessary
    rat_model_fix_bounds(model);
    
#ifndef NO_DISPLAY_LISTS
    // Compile display list for first frame only (on platforms that support display lists)
    rat_model_compile_first_frame_display_list(model);
#endif

    num_rat_models++;
    
#ifndef N64
    printf("RAT Handler: Created model '%s' (slot %d) - %u frames, %u vertices\n", 
           name, slot, model->total_frames, model->context->num_vertices);
#endif
    
    return model;
}

RatModel* rat_model_get(const char* name) {
    int slot = find_model_by_name(name);
    if (slot >= 0) {
        return &rat_models[slot];
    }
    return NULL;
}

void audio_poll(void);
void rat_model_destroy(RatModel* model) {
    if (!model || !model->is_valid) return;
    
        #ifdef N64
            audio_poll();
        #endif
    // Clean up display lists first
    rat_model_cleanup_first_frame_display_list(model);
    
        #ifdef N64
            audio_poll();
        #endif
    // Clean up resources
    if (model->context) {
        rat_free_context(model->context);
        model->context = NULL;
    }
    
        #ifdef N64
            audio_poll();
        #endif
    if (model->anim_info) {
        rat_free_animation(model->anim_info);
        model->anim_info = NULL;
    }
    
        #ifdef N64
            audio_poll();
        #endif
#ifndef N64
    printf("RAT Handler: Destroyed model '%s'\n", model->name);
#endif
    
    // Mark slot as free
    model->is_valid = false;
    num_rat_models--;
}

bool rat_model_destroy_by_name(const char* name) {
    RatModel* model = rat_model_get(name);
    if (model) {
        rat_model_destroy(model);
        return true;
    }
    return false;
}

// --- Transform Functions ---

void rat_model_set_position(RatModel* model, float x, float y, float z) {
    if (!model || !model->is_valid) return;
    model->transform.position[0] = x;
    model->transform.position[1] = y;
    model->transform.position[2] = z;
}

void rat_model_set_rotation(RatModel* model, float x_rad, float y_rad, float z_rad) {
    if (!model || !model->is_valid) return;
    model->transform.rotation[0] = x_rad;
    model->transform.rotation[1] = y_rad;
    model->transform.rotation[2] = z_rad;
}

void rat_model_set_scale(RatModel* model, float x, float y, float z) {
    if (!model || !model->is_valid) return;
    model->transform.scale[0] = x;
    model->transform.scale[1] = y;
    model->transform.scale[2] = z;
}

void rat_model_set_uniform_scale(RatModel* model, float scale) {
    rat_model_set_scale(model, scale, scale, scale);
}

void rat_model_get_position(RatModel* model, float* x, float* y, float* z) {
    if (!model || !model->is_valid) return;
    if (x) *x = model->transform.position[0];
    if (y) *y = model->transform.position[1];
    if (z) *z = model->transform.position[2];
}

void rat_model_get_rotation(RatModel* model, float* x, float* y, float* z) {
    if (!model || !model->is_valid) return;
    if (x) *x = model->transform.rotation[0];
    if (y) *y = model->transform.rotation[1];
    if (z) *z = model->transform.rotation[2];
}

void rat_model_get_scale(RatModel* model, float* x, float* y, float* z) {
    if (!model || !model->is_valid) return;
    if (x) *x = model->transform.scale[0];
    if (y) *y = model->transform.scale[1];
    if (z) *z = model->transform.scale[2];
}

// --- Animation Control ---
uint32_t roll = 0;

void rat_model_update(RatModel* model, uint32_t frame) {
    if (!model || !model->is_valid || !model->context) return;
    
    // Clamp frame to valid range
    if (frame >= model->total_frames) {
        frame = frame % model->total_frames;
    }
    // Only update if frame changed
    if (frame != model->current_frame) {  
        rat_decompress_to_frame(model->context, frame);
        model->current_frame = frame;
    }
}

// Function to update with logical frame (handles frame skipping internally)
void rat_model_update_logical_frame(RatModel* model, uint32_t logical_frame) {
#ifdef N64
    if (model && model->frame_skipping_enabled) {
        // Convert logical frame to actual frame
        uint32_t effective_frames = model->effective_total_frames;
        logical_frame = logical_frame % effective_frames;
        uint32_t actual_frame = rat_model_logical_to_actual_frame(model, logical_frame);
        rat_model_update(model, actual_frame);
    } else {
        rat_model_update(model, logical_frame);
    }
#else
    rat_model_update(model, logical_frame);
#endif
}

void rat_model_set_frame(RatModel* model, uint32_t frame) {
    rat_model_update(model, frame);
}

uint32_t rat_model_get_frame(RatModel* model) {
    if (!model || !model->is_valid) return 0;
    return model->current_frame;
}

uint32_t rat_model_get_total_frames(RatModel* model) {
    if (!model || !model->is_valid) return 0;
    return model->total_frames;
}

// Get effective frame count (may be reduced due to N64 frame skipping)
uint32_t rat_model_get_effective_frames(RatModel* model) {
    if (!model || !model->is_valid) return 0;
#ifdef N64
    return model->effective_total_frames;
#else
    return model->total_frames;
#endif
}

// --- Rendering ---

void rat_model_render(RatModel* model) {
    if (!model || !model->is_valid || !model->context) return;

#if !defined(NO_DISPLAY_LISTS)
    #if defined(N64)
        // On N64, try to compile the first frame display list if not yet done
        extern void* state;
        if (state != NULL && !model->first_frame_compiled && !model->failed_display_list) {
            rat_model_compile_first_frame_display_list(model);
        }
    #else

        // If the first frame display list wasn't compiled yet (e.g. deferred), try now.
        if (!model->first_frame_compiled) {
            rat_model_compile_first_frame_display_list(model);
        }
    #endif
#endif
    
    glPushMatrix();
    
    // Apply transformations
    glTranslatef(model->transform.position[0], 
                model->transform.position[1], 
                model->transform.position[2]);
    
    // Apply rotations (order: Z, Y, X)
    if (model->transform.rotation[2] != 0.0f) 
        glRotatef(model->transform.rotation[2] * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
    if (model->transform.rotation[1] != 0.0f) 
        glRotatef(model->transform.rotation[1] * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
    if (model->transform.rotation[0] != 0.0f) 
        glRotatef(model->transform.rotation[0] * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
    
    // Apply scale
    glScalef(model->transform.scale[0], 
            model->transform.scale[1],  
            model->transform.scale[2]);
    
    // Bind texture if available 
    if (model->texture_id > 0) {
        bind_texture(model->texture_id);
        glEnable(GL_TEXTURE_2D);
    }
    
    // Setup rendering state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    //glShadeModel(GL_SMOOTH);
    //glEnable(GL_COLOR_MATERIAL);
     
    if (model->texture_id > 0) {
        //glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
    }
    // Try to render using display list for the current frame
    if (USE_DISPLAY_LISTS(model)) {
#ifndef NO_DISPLAY_LISTS
        rat_model_render_display_list(model, model->current_frame);
#endif
    } else {
#ifdef NO_DISPLAY_LISTS
        if (model->current_frame == 0 && model->first_frame_compiled) {
            rat_render_display_list_safe(model->first_frame_display_list);
        } else {
            rat_render_opengl(model->context, GL_TRIANGLES);
        }
#else
        // Fall back to immediate mode rendering if display lists are not ready/failed
        rat_render_opengl(model->context, GL_TRIANGLES);
#endif
    }
    
    glPopMatrix();
}


void rat_model_render_wireframe(RatModel* model) {
    if (!model || !model->is_valid || !model->context) return;
    
    glPushMatrix();
    
    // Apply transformations (same as regular render)
    glTranslatef(model->transform.position[0], 
                model->transform.position[1], 
                model->transform.position[2]);
    
    if (model->transform.rotation[2] != 0.0f) 
        glRotatef(model->transform.rotation[2] * 180.0f / M_PI, 0.0f, 0.0f, 1.0f);
    if (model->transform.rotation[1] != 0.0f) 
        glRotatef(model->transform.rotation[1] * 180.0f / M_PI, 0.0f, 1.0f, 0.0f);
    if (model->transform.rotation[0] != 0.0f) 
        glRotatef(model->transform.rotation[0] * 180.0f / M_PI, 1.0f, 0.0f, 0.0f);
    
    glScalef(model->transform.scale[0], 
            model->transform.scale[1], 
            model->transform.scale[2]);
    
    // Disable texture for wireframe
    glDisable(GL_TEXTURE_2D);
    
    // Set wireframe mode
    glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
    
    // Setup rendering state
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
    glColor3f(1.0f, 1.0f, 1.0f); // White wireframe
    
    // Render using display list if available, otherwise fall back to rat_render_opengl
    if (USE_DISPLAY_LISTS(model)) {
#ifndef NO_DISPLAY_LISTS
        rat_model_render_display_list(model, model->current_frame);
#endif
    } else {
#ifdef NO_DISPLAY_LISTS
        if (model->current_frame == 0 && model->first_frame_compiled) {
            rat_render_display_list_safe(model->first_frame_display_list);
        } else {
            // Fall back to immediate mode rendering
            rat_render_opengl(model->context, GL_TRIANGLES);
        }
#else
        // Fall back to immediate mode rendering
        rat_render_opengl(model->context, GL_TRIANGLES);
#endif
    }
    
    glPopMatrix();
}

// --- Utility Functions ---

void rat_model_print_info(RatModel* model) {
    if (!model || !model->is_valid) {
#ifndef N64
        printf("RAT Model: Invalid model\n");
#endif
        return;
    }
    
#ifndef N64
    printf("RAT Model Info: '%s'\n", model->name);
    printf("  Frames: %u/%u\n", model->current_frame, model->total_frames);
    printf("  Vertices: %u\n", model->context->num_vertices);
    printf("  Texture ID: %d\n", model->texture_id);
    printf("  Position: (%.2f, %.2f, %.2f)\n", 
           model->transform.position[0], 
           model->transform.position[1], 
           model->transform.position[2]);
    printf("  Rotation: (%.2f, %.2f, %.2f) rad\n", 
           model->transform.rotation[0], 
           model->transform.rotation[1], 
           model->transform.rotation[2]);
    printf("  Scale: (%.2f, %.2f, %.2f)\n", 
           model->transform.scale[0], 
           model->transform.scale[1], 
           model->transform.scale[2]);
    printf("  Bounds: X[%.2f,%.2f] Y[%.2f,%.2f] Z[%.2f,%.2f]\n",
           model->anim_info->min_x, model->anim_info->max_x,
           model->anim_info->min_y, model->anim_info->max_y,
           model->anim_info->min_z, model->anim_info->max_z);
#ifdef N64
    if (model->frame_skipping_enabled) {
        printf("  N64 Frame Skipping: ENABLED (every %u frames, %u->%u effective)\n",
               model->frame_skip_factor, model->total_frames, model->effective_total_frames);
    } else {
        printf("  N64 Frame Skipping: DISABLED\n");
    }
#endif
#ifndef NO_DISPLAY_LISTS
    printf("  Display Lists: %s (frame 0 only)\n", model->first_frame_compiled ? "Compiled" : "Not compiled");
#else
    printf("  Display Lists: %s (frame 0 only)\n", model->first_frame_compiled ? "Compiled" : "Not compiled");
#endif
#endif
}

void rat_handler_print_all_models(void) {
    printf("RAT Handler: %d active models (max %d)\n", num_rat_models, MAX_RAT_MODELS);
    
    for (int i = 0; i < MAX_RAT_MODELS; i++) {
        if (rat_models[i].is_valid) {
            printf("  [%d] %s - Frame %u/%u\n", 
                   i, rat_models[i].name, 
                   rat_models[i].current_frame, 
                   rat_models[i].total_frames);
        }
    }
}

int rat_handler_get_model_count(void) {
    return num_rat_models;
}

bool rat_model_is_valid(RatModel* model) {
    return model && model->is_valid;
}

const char* rat_model_get_name(RatModel* model) {
    if (!model || !model->is_valid) return NULL;
    return model->name;
}

uint32_t rat_model_get_triangle_count(RatModel* model) {
    if (!model || !model->is_valid || !model->context) {
        return 0;
    }
    
    return rat_get_triangle_count(model->context);
}

// --- Display List Public Functions ---
// Updated for simplified first-frame-only approach

bool rat_model_are_display_lists_compiled(RatModel* model) {
    if (!model || !model->is_valid) {
        return false;
    }
    return model->first_frame_compiled;
}

uint32_t rat_model_get_compiled_frames_count(RatModel* model) {
    if (!model || !model->is_valid) {
        return 0;
    }
    // Only frame 0 is compiled now
    return model->first_frame_compiled ? 1 : 0;
}

bool rat_model_is_gradual_compilation_active(RatModel* model) {
    // No gradual compilation anymore - always false
    return false;
}

void rat_model_recompile_display_lists(RatModel* model) {
    if (!model || !model->is_valid) {
        return;
    }
#ifndef NO_DISPLAY_LISTS
    rat_model_compile_first_frame_display_list(model);
#endif
}

void rat_model_force_immediate_mode(RatModel* model, bool force) {
    if (!model || !model->is_valid) {
        return;
    }
    model->force_immediate_mode = force;
}

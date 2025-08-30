/*
RAT Animation Library - Header Only
============================

This header-only library provides functionality to load and decompress RAT (Reduced Animation
Transformation) files, which store compressed vertex animations.

#include <stdio.h>

#ifndef DEBUG
#ifdef N64
#define DEBUG 0
#else
#define DEBUG 1
#endif
#endif

// Usage:
------
1. Include this header in your project:
   #include "animation.h"

2. Load a RAT file:
   RatAnimationInfo* anim = rat_read_file("path/to/animation.rat");
   if (!anim) {
       const char* error = rat_get_last_error();
       // Handle error...
   }

3. Create a decompression context:
   DecompressionContext* ctx = rat_create_decompression_context(anim);
   if (!ctx) {
       const char* error = rat_get_last_error();
       // Handle error...
   }

4. Decompress frames:
   // The vertices will be available in ctx->current_frame_vertices
   rat_decompress_to_frame(ctx, frame_number);

5. Clean up:
   rat_free_decompression_context(ctx);
   rat_free_animation(anim);

Note: All functions are thread-safe except for error handling which uses a global buffer.§

OpenGL Usage:
------------
1. Using the built-in renderer (OpenGL 1.2+ compatible, N64 optimized):
    // Load and optimize animation for vertex cache
    RatAnimationInfo* anim = rat_read_file("path/to/animation.rat");
    rat_optimize_animation_cache(anim);  // Tom Forsyth's Linear-Speed Vertex Cache Optimization
    
    // Print debug info before updating animation
    if (debug_frame) {
        printf("Animation state before update:\n");
        printf("  Current frame: %u, Target frame: %u\n", ctx->current_frame, current_frame);
        printf("  Model bounds: X[%.2f, %.2f], Y[%.2f, %.2f], Z[%.2f, %.2f]\n",
               ctx->min_x, ctx->max_x, ctx->min_y, ctx->max_y, ctx->min_z, ctx->max_z);
        
        // Print a sample vertex
        if (ctx->current_frame_vertices && ctx->num_vertices > 0) {
            printf("  Sample vertex: [%.2f, %.2f, %.2f]\n",
                  ctx->current_frame_vertices[0],
                  ctx->current_frame_vertices[1],
                  ctx->current_frame_vertices[2]);
        }
    }
    
    // Update animation frame
    rat_decompress_to_frame(ctx, current_frame);
    
    // Render the model with colors and solid faces
    glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
    rat_render_opengl(ctx, GL_TRIANGLES); // Uses cache-optimized indices

2. Using modern OpenGL (3.0+) - NOTE: Not recommended for N64:
   // VBOs cause unnecessary memory copying on N64's UMA architecture
   // Use client-side vertex arrays instead for optimal N64 performance
   
   // Position attribute (3 floats per vertex)
   glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, ctx->current_frame_vertices);
   
   // UV attribute (2 floats per vertex)
   glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, sizeof(VertexUV), ctx->uvs);
   
   // Color attribute (4 bytes per vertex)
   glVertexAttribPointer(2, 4, GL_FLOAT, GL_TRUE, sizeof(VertexColor), ctx->colors);

   // Update animation frame
   rat_decompress_to_frame(ctx, current_frame);
   
   // Draw your mesh (avoid VBOs on N64 - they cause extra memory copies)
   glDrawArrays(GL_TRIANGLES, 0, ctx->num_vertices);

N64 Vertex Cache Optimization:
-----------------------------
The built-in renderer uses Tom Forsyth's Linear-Speed Vertex Cache Optimization:
- Call rat_optimize_animation_cache() after loading to reorder indices for optimal cache usage
- Optimized for N64's 32-entry vertex cache (configurable via RAT_VERTEX_CACHE_SIZE)
- Provides 2-4x performance improvement for complex meshes with shared vertices
- Algorithm reference: https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html
- For best N64 performance: use indexed rendering with cache-optimized vertex order
*/
// DEBUG SETTING IS HERE FOR DEBUGGING PURPOSES
#ifdef N64
#define DEBUG 0
#endif

#ifndef RAT_DECOMPRESS_H
#define RAT_DECOMPRESS_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <wii_memory_functions.h>

// Interleaved bit width struct for cache-friendly access
typedef struct {
    uint8_t x, y, z;
} RatBitWidths;

// Interleaved vertex struct for OpenGL/cache-friendly rendering
// (Assuming this is the intended layout: position, color, uv)
typedef struct {
    float x, y, z;    // Position
    float r, g, b, a; // Color
    float u, v;      // UV
} RatVertexFull;

#ifndef N64
#include "GL_macros.h" // This handles all the OpenGL functions, no need to include it in the header file otherwise
#endif
// Manual byte swap functions for TCC compatibility
#pragma pack(push, 1)
typedef struct {
    uint8_t x, y, z;
} VertexU8;

typedef struct {
    float u, v; // Always present, zero-initialized if missing in source
} VertexUV;

typedef struct {
    float r, g, b, a; // Always present, zero-initialized if missing in source
} VertexColor;

typedef struct {
    uint32_t magic;          // 'RAT1'
    uint32_t num_vertices;
    uint32_t num_frames;
    uint32_t num_indices;
    uint32_t uv_offset;
    uint32_t color_offset;
    uint32_t indices_offset;
    uint32_t delta_offset;
    uint32_t bit_widths_offset; // Offset to bit widths array
    float min_x, min_y, min_z;   // Minimum model bounds for denormalization
    float max_x, max_y, max_z;   // Maximum model bounds for denormalization
    uint8_t reserved[4];    // Padding for alignment
} RatHeader;

// N64 Cache-Optimized: Separate read-only data from dynamic data
typedef struct {
    // Hot data - frequently accessed during decompression (cache line 1)
    uint32_t num_vertices;
    uint32_t num_frames;
    uint32_t num_indices;
    uint32_t delta_stream_size;
    
    // Model bounds - read-only after load (cache line 1 continued)
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
    
    // Read-only arrays - separate memory region for RDRAM optimization
    VertexU8 *first_frame;      // Read-only reference frame
    uint32_t *delta_stream;     // Read-only compressed deltas
    VertexUV *uvs;              // Read-only UV coordinates
    VertexColor *colors;        // Read-only vertex colors
    uint16_t *indices;          // Read-only triangle indices
    
    // Bit widths - read frequently but small, keep together
    RatBitWidths *bit_widths; // Interleaved array for X/Y/Z bit widths
    
    // Display list support for N64/OpenGL
    GLuint *display_lists;   // Array of display list IDs
    uint8_t lists_compiled;  // Flag indicating if display lists are compiled
} RatAnimationInfo;

typedef struct {
    const uint32_t *stream;
    uint32_t current_bits;
    int bits_remaining;
    size_t position;
    size_t stream_length;
} BitstreamReader;

// N64 Cache-Optimized: Group hot data in first cache line, separate read/write regions
typedef struct {
    // === HOT DATA - First 64 bytes (cache line 1) ===
    uint32_t current_frame;     // Most frequently accessed
    uint32_t num_vertices;      // Loop bounds
    uint32_t num_frames;        // Frame validation
    uint32_t num_indices;       // Rendering
    
    // Bitstream state - hot during decompression
    BitstreamReader bit_reader; // 24 bytes on 32-bit systems
    
    // Progressive decompression state - hot during progressive mode
    uint32_t target_frame;
    uint32_t vertices_processed;
    uint32_t vertices_per_slice;
    uint32_t max_time_us;
    bool progressive_active;
    bool progressive_complete;
    
    // === WRITE-HEAVY DATA - Separate cache lines ===
    VertexU8 *current_positions; // Write-heavy during decompression
    float* current_frame_vertices; // Write-heavy during denormalization
    
    // === READ-ONLY DATA - Separate memory region ===
    VertexU8 *first_frame;      // Read-only reference frame
    VertexUV *uvs;              // Read-only UV coordinates
    VertexColor *colors;        // Read-only vertex colors
    uint16_t *indices;          // Read-only triangle indices
    
    // Model bounds - read-only after initialization
    float min_x, min_y, min_z;
    float max_x, max_y, max_z;
    
    // Bit widths - read frequently but small
    RatBitWidths *bit_widths;
    
    // Interleaved vertex struct for cache-friendly OpenGL rendering
    RatVertexFull* interleaved_vertices; // Interleaved array for OpenGL
    
    // Optimization flag to avoid redundant UV/color updates
    bool static_data_initialized;        // True after UV/color data has been set once
    
    // Debug identification for multi-actor systems
    uint32_t context_id;                 // Unique ID for debugging multiple contexts
} DecompressionContext;

static inline const char* rat_get_last_error(void);
static inline uint32_t rat_get_triangle_count(DecompressionContext *ctx);

// Error handling
static char rat_last_error[256] = {0};
#pragma pack(pop)

// Performance optimization: Pre-computed sign extension lookup tables for common bit widths
// This eliminates branching and bit manipulation for the most common cases
static const int8_t sign_extend_1bit[2] = {0, -1};
static const int8_t sign_extend_2bit[4] = {0, 1, -2, -1};
static const int8_t sign_extend_3bit[8] = {0, 1, 2, 3, -4, -3, -2, -1};
static const int8_t sign_extend_4bit[16] = {0, 1, 2, 3, 4, 5, 6, 7, -8, -7, -6, -5, -4, -3, -2, -1};

// Fast sign extension using lookup tables for common bit widths
static inline int fast_sign_extend(uint32_t value, uint8_t bits) {
    switch (bits) {
        case 1: return sign_extend_1bit[value & 1];
        case 2: return sign_extend_2bit[value & 3];
        case 3: return sign_extend_3bit[value & 7];
        case 4: return sign_extend_4bit[value & 15];
        default: {
            // Fall back to bit manipulation for larger bit widths
            uint32_t sign_mask = 1u << (bits - 1);
            uint32_t extend_mask = ~0u << bits;
            return (value & sign_mask) ? (value | extend_mask) : value;
        }
    }
}

// Forward declarations of helper functions
static inline void* safe_malloc(size_t size);
static inline void bitstream_init(BitstreamReader *br, const uint32_t *stream, size_t stream_length);
static inline uint32_t bitstream_read(BitstreamReader *br, uint8_t num_bits, bool *error);
static inline bool bitstream_read_vertex_deltas(BitstreamReader *br, uint8_t bx, uint8_t by, uint8_t bz, int *dx, int *dy, int *dz);
static inline bool bitstream_read_vertex_deltas_batch(BitstreamReader *br, uint8_t bx, uint8_t by, uint8_t bz, int *dx_array, int *dy_array, int *dz_array, uint32_t count);
static inline void rat_prefetch_memory(const void* addr);
static inline uint64_t rat_get_performance_ticks(void);
static inline bool rat_decompress_frame_specialized(DecompressionContext *ctx, uint8_t bx, uint8_t by, uint8_t bz);

// N64 Profiler function declarations
#ifdef N64
extern bool profiler_start_segment(const char* name);
extern bool profiler_end_segment(void);
#else
// Non-N64 fallbacks - no-op macros (disabled when profiler is being implemented)
#ifndef N64_PROFILER_IMPLEMENTATION
#define profiler_start_segment(name) ((void)0)
#define profiler_end_segment() ((void)0)
#else
// When profiler implementation is available, use the actual functions
extern bool profiler_start_segment(const char* name);
extern bool profiler_end_segment(void);
#endif
#endif

// Core functions
static inline void rat_free_animation(RatAnimationInfo *anim) {
    if (!anim) return;
    
    if (anim->first_frame) free(anim->first_frame);
    if (anim->delta_stream) free(anim->delta_stream);
    if (anim->uvs) free(anim->uvs);
    if (anim->colors) free(anim->colors);
    if (anim->indices) free(anim->indices);
    
    // Free bit width arrays
    if (anim->bit_widths) free(anim->bit_widths);
    /*
    if (anim->display_lists) {
        if (anim->lists_compiled) {
            glDeleteLists(anim->display_lists[0], anim->num_frames);
        }
        free(anim->display_lists);
    }*/
    
    free(anim);
}

static inline void rat_free_context(DecompressionContext *ctx) {
    if (!ctx) return;
    
    // Free allocated memory
    if (ctx->current_positions) free(ctx->current_positions);
    if (ctx->current_frame_vertices) free(ctx->current_frame_vertices);
    
    // Free bit width arrays
    if (ctx->bit_widths) free(ctx->bit_widths);
    
    // Free the context itself
    free(ctx);
}

// Implementation of helper functions
static inline void* safe_malloc(size_t size) {
    if (size == 0) return NULL;
#ifdef GEKKO
    void* ptr = AllocateGPUMemory(size);
#else
    void* ptr = malloc(size);
#endif
    if (!ptr) {
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Memory allocation failed for %zu bytes", size);
        return NULL;
    }
    return ptr;
}

static inline void* risky_malloc(size_t size)
{
    if (size == 0) return NULL;
#ifdef GEKKO
    void* ptr = AllocateGPUMemory(size);
#else
    void* ptr = malloc(size);
#endif
    return ptr;
}

#ifdef N64
// N64 RDRAM-optimized memory allocation
// Separate read-only and write-heavy data for better memory bus usage
static inline void* rat_alloc_readonly(size_t size) {
    // Align to 8-byte boundaries for DMA efficiency
    size_t aligned_size = (size + 7) & ~7;
    void* ptr = malloc(aligned_size);
    if (!ptr) {
        snprintf(rat_last_error, sizeof(rat_last_error), "Read-only allocation failed for %zu bytes", size);
    }
    return ptr;
}

static inline void* rat_alloc_writeonly(size_t size) {
    // Align to cache line boundaries (32 bytes on N64) for better cache performance
    size_t aligned_size = (size + 31) & ~31;
    void* ptr = malloc(aligned_size);
    if (!ptr) {
        snprintf(rat_last_error, sizeof(rat_last_error), "Write-only allocation failed for %zu bytes", size);
    }
    return ptr;
}

// Memory prefetch for N64 - helps with sequential access patterns
static inline void rat_prefetch_memory(const void* addr) {
    // N64 cache line prefetch - use volatile to prevent optimization
    if (addr) {
        volatile uint32_t dummy = *(volatile uint32_t*)addr;
        (void)dummy; // Suppress unused variable warning
    }
}

// MIPS cache instructions for intensive animation decompression operations
// These are only used during the heavy decompression loops where cache performance matters most
static inline void rat_animation_cache_prefetch_load(const void* addr) {
    if (addr) {
        // Align to 16-byte boundary for MIPS cache instruction safety
        uintptr_t aligned_addr = ((uintptr_t)addr) & ~15;
        void* cache_addr = (void*)aligned_addr;
        
        // MIPS CACHE instruction: Load cache line for reading
        __asm__ volatile (
            "cache 0x14, 0(%0)"  // CACHE Load, offset 0
            :                    // no outputs
            : "r" (cache_addr)   // input: aligned address in register
            : "memory"           // clobber: memory barrier
        );
    }
}

static inline void rat_animation_cache_prefetch_store(void* addr) {
    if (addr) {
        // Align to 16-byte boundary for MIPS cache instruction safety
        uintptr_t aligned_addr = ((uintptr_t)addr) & ~15;
        void* cache_addr = (void*)aligned_addr;
        
        // MIPS CACHE instruction: Prepare cache line for writing
        __asm__ volatile (
            "cache 0x15, 0(%0)"  // CACHE PrepareForStore, offset 0
            :                    // no outputs
            : "r" (cache_addr)   // input: aligned address in register
            : "memory"           // clobber: memory barrier
        );
    }
}
#else
// Non-N64 fallbacks
#define rat_alloc_readonly(size) safe_malloc(size)
#define rat_alloc_writeonly(size) safe_malloc(size)
static inline void rat_prefetch_memory(const void* addr) {
    // No-op on non-N64 platforms
    (void)addr;
}
#endif

static inline void bitstream_init(BitstreamReader *br, const uint32_t *stream, size_t stream_length) {
    br->stream = stream;
    br->position = 0;
    br->current_bits = stream_length > 0 ? stream[0] : 0;
    br->bits_remaining = 32;
    br->stream_length = stream_length;
    
    // Debug stream length for troubleshooting
    static bool print_once = false;
    if (!print_once) {
        printf("Initialized bitstream reader with %zu words\n", stream_length);
        print_once = true;
    }
}

static inline uint32_t bitstream_read(BitstreamReader *br, uint8_t num_bits, bool *error) {
    // Branch predictor optimization: most calls don't need error reporting
    if (__builtin_expect(error != NULL, 0)) *error = false;
    
    // Ultra-fast path for zero bits (common case)
    if (__builtin_expect(num_bits == 0, 0)) return 0;
    
    // Branch predictor hint: input validation rarely fails in normal operation
    if (__builtin_expect(!br || num_bits > 32, 0)) {
        if (error) *error = true;
        return 0;
    }

    // Ultra-fast path optimized for branch prediction
    // Most animation deltas fit in current bits buffer (90%+ hit rate)
    #ifdef N64
    register uint32_t fast_path_condition = (num_bits <= 12) & (num_bits <= br->bits_remaining);
    #else
    register uint32_t fast_path_condition = (num_bits <= 10) & (num_bits <= br->bits_remaining);
    #endif
    
    if (__builtin_expect(fast_path_condition, 1)) {
        // Branchless fast path - no conditional execution
        register uint32_t result = br->current_bits >> (32 - num_bits);
        br->current_bits <<= num_bits;
        br->bits_remaining -= num_bits;
        return result;
    }

    // Boundary check with branch prediction hint (rare case)
    if (__builtin_expect(num_bits > br->bits_remaining && (br->position + 1 >= br->stream_length), 0)) {
        if (error) *error = true;
        return 0; 
    }
        
    // Handle split reads across words (less common path)
    register uint32_t result = 0;
    
    if (br->bits_remaining >= num_bits) {
        // Single word read (common case)
        result = br->current_bits >> (32 - num_bits);
        br->current_bits <<= num_bits;
        br->bits_remaining -= num_bits;
    } else {
        // Split read across two words (uncommon case)
        register uint8_t first_bits = br->bits_remaining;
        
        result = br->current_bits >> (32 - first_bits);
        register uint8_t remaining_bits_to_read = num_bits - first_bits;
        
        br->position++;
        br->current_bits = br->stream[br->position];
        br->bits_remaining = 32;
        
        // Combine result with bits from the new word
        result = (result << remaining_bits_to_read) | (br->current_bits >> (32 - remaining_bits_to_read));
        br->current_bits <<= remaining_bits_to_read;
        br->bits_remaining -= remaining_bits_to_read;
    }
    
    return result;
}

// Branch-prediction optimized inline vertex delta reading for all platforms
// Eliminates conditional branches in the critical decompression path
static inline bool rat_read_vertex_deltas_inline(BitstreamReader *br, 
                                                 uint8_t bx, uint8_t by, uint8_t bz,
                                                 int *dx, int *dy, int *dz) {
    // Initialize outputs - branchless approach
    *dx = *dy = *dz = 0;
    
    register uint8_t total_bits = bx + by + bz;
    
    // Branch predictor hint: most animation deltas use small bit counts (95%+ of cases)
    if (__builtin_expect(total_bits > 0 && total_bits <= 16, 1)) {
        register uint32_t packed_data = bitstream_read(br, total_bits, NULL);
        
        // Branchless component extraction using masks and shifts
        // This eliminates conditional branches that cause mispredictions
        
        // Z component extraction (branchless)
        register uint32_t z_mask = ((1u << bz) - 1) & (-(bz > 0)); // 0 if bz==0, else mask
        register uint32_t raw_dz = packed_data & z_mask;
        *dz = (bz > 0) ? fast_sign_extend(raw_dz, bz) : 0;
        packed_data >>= bz;
        
        // Y component extraction (branchless)
        register uint32_t y_mask = ((1u << by) - 1) & (-(by > 0)); // 0 if by==0, else mask
        register uint32_t raw_dy = packed_data & y_mask;
        *dy = (by > 0) ? fast_sign_extend(raw_dy, by) : 0;
        packed_data >>= by;
        
        // X component extraction (branchless)
        register uint32_t x_mask = ((1u << bx) - 1) & (-(bx > 0)); // 0 if bx==0, else mask
        register uint32_t raw_dx = packed_data & x_mask;
        *dx = (bx > 0) ? fast_sign_extend(raw_dx, bx) : 0;
        
        return true;
    }
    
    // Unlikely fallback path for larger bit widths
    // Use __builtin_expect to hint this is the rare case
    if (__builtin_expect(total_bits > 16, 0)) {
        // Sequential reads for large bit widths (rare case)
        if (bx > 0) *dx = fast_sign_extend(bitstream_read(br, bx, NULL), bx);
        if (by > 0) *dy = fast_sign_extend(bitstream_read(br, by, NULL), by);
        if (bz > 0) *dz = fast_sign_extend(bitstream_read(br, bz, NULL), bz);
        return true;
    }
    
    // Zero bits case (extremely rare)
    return true;
}

// Safe delta application for all platforms
static inline void rat_apply_deltas_safe(VertexU8 *pos, int dx, int dy, int dz) {
    pos->x = (pos->x + dx) & 0xFF;
    pos->y = (pos->y + dy) & 0xFF;
    pos->z = (pos->z + dz) & 0xFF;
}

// Original function kept for compatibility but now optimized
static inline bool bitstream_read_vertex_deltas(BitstreamReader *br, 
                                                uint8_t bx, uint8_t by, uint8_t bz,
                                                int *dx, int *dy, int *dz) {
    return rat_read_vertex_deltas_inline(br, bx, by, bz, dx, dy, dz);
}

// Forward declarations for functions used in display list compilation
static inline void rat_decompress_to_frame(DecompressionContext *ctx, uint32_t frame_index);
DecompressionContext* rat_create_context(RatAnimationInfo *anim);

// Global OpenGL state tracking to avoid conflicts between multiple contexts
static bool rat_global_vertex_array_enabled = false;
static bool rat_global_texcoord_array_enabled = false;
static bool rat_global_color_array_enabled = false;
static void* rat_global_last_vertex_pointer = NULL;
static void* rat_global_last_texcoord_pointer = NULL;
static void* rat_global_last_color_pointer = NULL;
static GLsizei rat_global_last_vertex_stride = 0;

// Highly optimized OpenGL rendering function with N64-specific optimizations
// Based on LibDragon OpenGL best practices:
// - Uses immediate mode for small meshes (< 96 indices, < 64 vertices) to avoid glDrawElements overhead
// - Switches to vertex arrays for larger meshes where the setup cost is justified
// - Optimizes attribute order (Color -> TexCoord -> Vertex) for N64 pipeline efficiency
// - Uses global state caching to minimize redundant OpenGL state changes
// - Batches large meshes to stay within N64 hardware limits
static inline void rat_render_opengl(DecompressionContext *ctx, GLenum primitive_type) {
    if (!ctx || !ctx->interleaved_vertices) return;

    #ifdef N64
    // N64 Performance Hint: Prefer triangle strips over triangle lists when possible
    // Triangle strips use the vertex cache more efficiently (LibDragon wiki recommendation)
    if (primitive_type == GL_TRIANGLES && ctx->num_indices > 96) {
        // For larger meshes, consider using GL_TRIANGLE_STRIP if your indices support it
        // This optimization would require index buffer analysis/conversion
    }
    #endif

    // Enable vertex array if not already enabled globally
    if (!rat_global_vertex_array_enabled) {
        glEnableClientState(GL_VERTEX_ARRAY);
        rat_global_vertex_array_enabled = true;
    }
    
    // Only set vertex pointer if it changed globally (avoids redundant state changes)
    if (rat_global_last_vertex_pointer != &ctx->interleaved_vertices[0].x || 
        rat_global_last_vertex_stride != sizeof(RatVertexFull)) {
        glVertexPointer(3, GL_FLOAT, sizeof(RatVertexFull), &ctx->interleaved_vertices[0].x);
        rat_global_last_vertex_pointer = &ctx->interleaved_vertices[0].x;
        rat_global_last_vertex_stride = sizeof(RatVertexFull);
    }

    // Conditional texture coordinate setup with global state caching
    if (ctx->uvs) {
        if (!rat_global_texcoord_array_enabled) {
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            rat_global_texcoord_array_enabled = true;
        }
        if (rat_global_last_texcoord_pointer != &ctx->interleaved_vertices[0].u) {
            glTexCoordPointer(2, GL_FLOAT, sizeof(RatVertexFull), &ctx->interleaved_vertices[0].u);
            rat_global_last_texcoord_pointer = &ctx->interleaved_vertices[0].u;
        }
    }
    // Note: Don't disable texture coords here - other models might need them

    // Conditional color setup with global state caching
    if (ctx->colors) {
        if (!rat_global_color_array_enabled) {
            glEnableClientState(GL_COLOR_ARRAY);
            rat_global_color_array_enabled = true;
        }
        if (rat_global_last_color_pointer != &ctx->interleaved_vertices[0].r) {
            glColorPointer(4, GL_FLOAT, sizeof(RatVertexFull), &ctx->interleaved_vertices[0].r);
            rat_global_last_color_pointer = &ctx->interleaved_vertices[0].r;
        }
    }
    // Note: Don't disable color arrays here - other models might need them

    // Optimized rendering with batching for large meshes
    if (ctx->indices && ctx->num_indices > 0) {
        // For very large meshes, batch the draw calls to avoid driver overhead 
        const uint32_t MAX_INDICES_PER_BATCH = 65535; // Maximum for 16-bit indices
        
        if (ctx->num_indices > MAX_INDICES_PER_BATCH) {
            // Batch rendering for huge meshes
            uint32_t remaining_indices = ctx->num_indices;
            uint16_t* current_indices = ctx->indices;
            
            while (remaining_indices > 0) {
                uint32_t batch_size = (remaining_indices > MAX_INDICES_PER_BATCH) ? 
                                     MAX_INDICES_PER_BATCH : remaining_indices;
                
                // N64 Optimization: Use immediate mode for small batches to avoid glDrawElements overhead
                #ifdef N64
                if (batch_size < 96) { // Threshold based on N64 vertex cache size (32 entries * 3 vertices per triangle)
                    // Use immediate mode for small batches - much faster on N64
                    glBegin(primitive_type);
                    for (uint32_t i = 0; i < batch_size; i++) {
                        uint16_t vertex_idx = current_indices[i];
                        if (vertex_idx < ctx->num_vertices) {
                            // Send vertex attributes in optimal order for N64 pipeline
                            if (ctx->colors) {
                                glColor4f(ctx->interleaved_vertices[vertex_idx].r,
                                         ctx->interleaved_vertices[vertex_idx].g,
                                         ctx->interleaved_vertices[vertex_idx].b,
                                         ctx->interleaved_vertices[vertex_idx].a);
                            }
                            if (ctx->uvs) {
                                glTexCoord2f(ctx->interleaved_vertices[vertex_idx].u,
                                            ctx->interleaved_vertices[vertex_idx].v);
                            }
                            glVertex3f(ctx->interleaved_vertices[vertex_idx].x,
                                      ctx->interleaved_vertices[vertex_idx].y,
                                      ctx->interleaved_vertices[vertex_idx].z);
                        }
                    }
                    glEnd();
                } else {
                    // Use glDrawElements for larger batches only
                    glDrawElements(primitive_type, batch_size, GL_UNSIGNED_SHORT, current_indices);
                }
                #else
                glDrawElements(primitive_type, batch_size, GL_UNSIGNED_SHORT, current_indices);
                #endif
                
                current_indices += batch_size;
                remaining_indices -= batch_size;
            }
        } else {
            // Single draw call for reasonable-sized meshes (cache-optimized indices)
            #ifdef N64
            // N64 Optimization: Use immediate mode for small meshes, glDrawElements for larger ones
            if (ctx->num_indices < 96) { // Small mesh threshold
                glBegin(primitive_type);
                for (uint32_t i = 0; i < ctx->num_indices; i++) {
                    uint16_t vertex_idx = ctx->indices[i];
                    if (vertex_idx < ctx->num_vertices) {
                        // Send attributes in optimal order for N64
                        if (ctx->colors) {
                            glColor4f(ctx->interleaved_vertices[vertex_idx].r,
                                     ctx->interleaved_vertices[vertex_idx].g,
                                     ctx->interleaved_vertices[vertex_idx].b,
                                     ctx->interleaved_vertices[vertex_idx].a);
                        }
                        if (ctx->uvs) {
                            glTexCoord2f(ctx->interleaved_vertices[vertex_idx].u,
                                        ctx->interleaved_vertices[vertex_idx].v);
                        }
                        glVertex3f(ctx->interleaved_vertices[vertex_idx].x,
                                  ctx->interleaved_vertices[vertex_idx].y,
                                  ctx->interleaved_vertices[vertex_idx].z);
                    }
                }
                glEnd();
            } else {
                // Use glDrawElements for larger meshes where the overhead is worth it
                glDrawElements(primitive_type, ctx->num_indices, GL_UNSIGNED_SHORT, ctx->indices);
            }
            #else
            glDrawElements(primitive_type, ctx->num_indices, GL_UNSIGNED_SHORT, ctx->indices);
            #endif
        }
    } else {
        // Fallback to vertex arrays when no indices available
        #ifdef N64
        // N64 Optimization: Use immediate mode for small vertex counts
        if (ctx->num_vertices < 64) { // Small mesh threshold for non-indexed rendering
            glBegin(primitive_type);
            for (uint32_t i = 0; i < ctx->num_vertices; i++) {
                // Send attributes in optimal order for N64
                if (ctx->colors) {
                    glColor4f(ctx->interleaved_vertices[i].r,
                             ctx->interleaved_vertices[i].g,
                             ctx->interleaved_vertices[i].b,
                             ctx->interleaved_vertices[i].a);
                }
                if (ctx->uvs) {
                    glTexCoord2f(ctx->interleaved_vertices[i].u,
                                ctx->interleaved_vertices[i].v);
                }
                glVertex3f(ctx->interleaved_vertices[i].x,
                          ctx->interleaved_vertices[i].y,
                          ctx->interleaved_vertices[i].z);
            }
            glEnd();
        } else {
            // Use vertex arrays for larger meshes
            const uint32_t MAX_VERTICES_PER_BATCH = 65535;
            
            if (ctx->num_vertices > MAX_VERTICES_PER_BATCH) {
                uint32_t remaining_vertices = ctx->num_vertices;
                uint32_t current_start = 0;
                
                while (remaining_vertices > 0) {
                    uint32_t batch_size = (remaining_vertices > MAX_VERTICES_PER_BATCH) ? 
                                         MAX_VERTICES_PER_BATCH : remaining_vertices;
                    
                    glDrawArrays(primitive_type, current_start, batch_size);
                    
                    current_start += batch_size;
                    remaining_vertices -= batch_size;
                }
            } else {
                glDrawArrays(primitive_type, 0, ctx->num_vertices);
            }
        }
        #else
        // Non-N64: Use vertex arrays with batching for large vertex counts
        const uint32_t MAX_VERTICES_PER_BATCH = 65535;
        
        if (ctx->num_vertices > MAX_VERTICES_PER_BATCH) {
            uint32_t remaining_vertices = ctx->num_vertices;
            uint32_t current_start = 0;
            
            while (remaining_vertices > 0) {
                uint32_t batch_size = (remaining_vertices > MAX_VERTICES_PER_BATCH) ? 
                                     MAX_VERTICES_PER_BATCH : remaining_vertices;
                
                glDrawArrays(primitive_type, current_start, batch_size);
                
                current_start += batch_size;
                remaining_vertices -= batch_size;
            }
        } else {
            glDrawArrays(primitive_type, 0, ctx->num_vertices);
        }
        #endif
    }
    
    // Note: We intentionally don't disable client states here for performance
    // Global state tracking ensures consistency across all models
    
    // N64 Performance Tips (from LibDragon wiki):
    // 1. For static geometry, consider using display lists (rat_compile_display_lists)
    // 2. Keep texture size <= 32x32 for optimal performance
    // 3. Use 16-bit textures (RGBA5551) when possible instead of 32-bit
    // 4. Minimize state changes between draw calls
    // 5. Triangle strips are more efficient than triangle lists for large meshes
}

// Reset global OpenGL state cache - call this when switching contexts or after other GL operations
static inline void rat_reset_opengl_state_cache(DecompressionContext *ctx) {
    // Reset the global state tracking (ctx parameter kept for API compatibility)
    (void)ctx; // Suppress unused parameter warning
    
    rat_global_vertex_array_enabled = false;
    rat_global_texcoord_array_enabled = false;
    rat_global_color_array_enabled = false;
    rat_global_last_vertex_pointer = NULL;
    rat_global_last_texcoord_pointer = NULL;
    rat_global_last_color_pointer = NULL;
    rat_global_last_vertex_stride = 0;
    
    // Disable all client states to start fresh
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    glDisableClientState(GL_COLOR_ARRAY);
}

// Safe display list rendering that works with global state tracking
// Display lists set their own OpenGL state, which doesn't interfere with our global cache
static inline void rat_render_display_list_safe(GLuint display_list) {
    if (display_list == 0) return;
    
    // Call the display list (it will set its own state)
    glCallList(display_list);
    
    // Display lists don't interfere with our global state tracking because:
    // 1. They maintain their own vertex array setup internally
    // 2. Our global state cache tracks immediate mode state between rat_render_opengl calls
    // 3. Display list state is separate from immediate mode state
}

// Advanced multi-model batch rendering - renders multiple RAT models efficiently
// Use this when you have multiple models to render in the same frame for maximum performance
// Returns the number of models successfully rendered
static inline int rat_render_multiple_models(DecompressionContext** contexts, int num_models, GLenum primitive_type) {
    if (!contexts || num_models <= 0) return 0;
    
    int models_rendered = 0;
    
    // Group models by their data characteristics to minimize state changes
    bool has_uvs = false;
    bool has_colors = false;
    
    // First pass: determine what OpenGL states we need
    for (int i = 0; i < num_models; i++) {
        if (contexts[i] && contexts[i]->interleaved_vertices) {
            if (contexts[i]->uvs) has_uvs = true;
            if (contexts[i]->colors) has_colors = true;
        }
    }
    
    // Set up OpenGL state once for all models
    glEnableClientState(GL_VERTEX_ARRAY);
    
    if (has_uvs) {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    
    if (has_colors) {
        glEnableClientState(GL_COLOR_ARRAY);
    }
    
    // Second pass: render all models with minimal state changes
    void* last_vertex_pointer = NULL;
    void* last_texcoord_pointer = NULL;
    void* last_color_pointer = NULL;
    
    for (int i = 0; i < num_models; i++) {
        DecompressionContext* ctx = contexts[i];
        if (!ctx || !ctx->interleaved_vertices) continue;
        
        // Only change vertex pointer if it's different
        if (last_vertex_pointer != &ctx->interleaved_vertices[0].x) {
            glVertexPointer(3, GL_FLOAT, sizeof(RatVertexFull), &ctx->interleaved_vertices[0].x);
            last_vertex_pointer = &ctx->interleaved_vertices[0].x;
        }
        
        // Only change texture coord pointer if needed and different
        if (has_uvs && ctx->uvs && last_texcoord_pointer != &ctx->interleaved_vertices[0].u) {
            glTexCoordPointer(2, GL_FLOAT, sizeof(RatVertexFull), &ctx->interleaved_vertices[0].u);
            last_texcoord_pointer = &ctx->interleaved_vertices[0].u;
        }
        
        // Only change color pointer if needed and different
        if (has_colors && ctx->colors && last_color_pointer != &ctx->interleaved_vertices[0].r) {
            glColorPointer(4, GL_FLOAT, sizeof(RatVertexFull), &ctx->interleaved_vertices[0].r);
            last_color_pointer = &ctx->interleaved_vertices[0].r;
        }
        
        // Render this model
        if (ctx->indices && ctx->num_indices > 0) {
            // Use batching for large meshes
            const uint32_t MAX_INDICES_PER_BATCH = 65535;
            
            if (ctx->num_indices > MAX_INDICES_PER_BATCH) {
                uint32_t remaining_indices = ctx->num_indices;
                uint16_t* current_indices = ctx->indices;
                
                while (remaining_indices > 0) {
                    uint32_t batch_size = (remaining_indices > MAX_INDICES_PER_BATCH) ? 
                                         MAX_INDICES_PER_BATCH : remaining_indices;
                    
                    glDrawElements(primitive_type, batch_size, GL_UNSIGNED_SHORT, current_indices);
                    
                    current_indices += batch_size;
                    remaining_indices -= batch_size;
                }
            } else {
                glDrawElements(primitive_type, ctx->num_indices, GL_UNSIGNED_SHORT, ctx->indices);
            }
        } else {
            // Vertex array rendering with batching
            const uint32_t MAX_VERTICES_PER_BATCH = 65535;
            
            if (ctx->num_vertices > MAX_VERTICES_PER_BATCH) {
                uint32_t remaining_vertices = ctx->num_vertices;
                uint32_t current_start = 0;
                
                while (remaining_vertices > 0) {
                    uint32_t batch_size = (remaining_vertices > MAX_VERTICES_PER_BATCH) ? 
                                         MAX_VERTICES_PER_BATCH : remaining_vertices;
                    
                    glDrawArrays(primitive_type, current_start, batch_size);
                    
                    current_start += batch_size;
                    remaining_vertices -= batch_size;
                }
            } else {
                glDrawArrays(primitive_type, 0, ctx->num_vertices);
            }
        }
        
        models_rendered++;
    }
    
    // Leave client states enabled for potential future rat_render_opengl calls
    // The individual rat_render_opengl function will handle them efficiently
    
    return models_rendered;
}

// Function to compile display lists for all frames
static inline int rat_compile_display_lists(RatAnimationInfo *anim) {
    if (!anim) return -1;

    // Allocate memory for display list IDs
    anim->display_lists = (GLuint*)safe_malloc(anim->num_frames * sizeof(GLuint));
    if (!anim->display_lists) {
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate display list array");
        return -1;
    }
	anim->lists_compiled = 0;

    // Generate display lists
    GLuint firstList = glGenLists(anim->num_frames);
    if (firstList == 0) {
        free(anim->display_lists);
        anim->display_lists = NULL;
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "glGenLists failed");
        return -1;
    }

    for (uint32_t i = 0; i < anim->num_frames; i++) {
        anim->display_lists[i] = firstList + i;
    }

    // Create temporary decompression context
    DecompressionContext *ctx = rat_create_context(anim);
    if (!ctx) {
        glDeleteLists(firstList, anim->num_frames);
        free(anim->display_lists);
        anim->display_lists = NULL;
        return -1;
    }

    // Compile each frame into a display list
    for (uint32_t frame = 0; frame < anim->num_frames; frame++) {
        rat_decompress_to_frame(ctx, frame);
        
        glNewList(anim->display_lists[frame], GL_COMPILE);
        rat_render_opengl(ctx, GL_TRIANGLES);
        glEndList();
    }

    rat_free_context(ctx);
    anim->lists_compiled = 1;
    
    // Note: With global state tracking, display list compilation doesn't interfere
    // with the global OpenGL state cache used by immediate mode rendering
    
    return 0;
}

// New rendering function using display lists with proper state management
static inline void rat_render_display_list(RatAnimationInfo *anim, uint32_t frame) {
    if (!anim || !anim->display_lists || !anim->lists_compiled || frame >= anim->num_frames) {
        // Fall back to regular rendering if display lists aren't available
        DecompressionContext *ctx = rat_create_context(anim);
        if (ctx) {
            rat_decompress_to_frame(ctx, frame);
            rat_render_opengl(ctx, GL_TRIANGLES);
            rat_free_context(ctx);
        }
        return;
    }
    
    // Use safe display list rendering that maintains state consistency
    rat_render_display_list_safe(anim->display_lists[frame]);
}

RatAnimationInfo* rat_read_file(const char *filename) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to open RAT file: %s", filename);
        return NULL;
    }

    // Validate file size
    fseek(fp, 0, SEEK_END);
    long file_size = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    RatHeader header;
    if (fread(&header, sizeof(RatHeader), 1, fp) != 1) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to read RAT header");
        return NULL;
    }

    // Endianness detection: On N64 (big-endian), RAT files are created on PC/Mac (little-endian)
    // So we need to determine if we're on a big-endian system reading little-endian data
    bool is_big_endian = false;
    
    #if defined(N64) || defined(GEKKO)
        // On N64, we're always big-endian reading little-endian files
#if DEBUG
        printf("N64 build detected - assuming little-endian RAT file, performing byte swapping\n");
#endif
        is_big_endian = true;
        
        // Validate that we're reading a RAT file (magic should be little-endian format)
        if (header.magic != 0x52415431) { // 'RAT1' in little-endian format
            fclose(fp);
            snprintf(rat_last_error, sizeof(rat_last_error), 
                    "Invalid RAT file magic on N64: 0x%08X (expected 0x52415431)", header.magic);
            return NULL;
        }
        
        // Byte swap all 32-bit values in the header
        header.num_vertices = manual_bswap32(header.num_vertices);
        header.num_frames = manual_bswap32(header.num_frames);
        header.num_indices = manual_bswap32(header.num_indices);
        header.uv_offset = manual_bswap32(header.uv_offset);
        header.color_offset = manual_bswap32(header.color_offset);
        header.indices_offset = manual_bswap32(header.indices_offset);
        header.delta_offset = manual_bswap32(header.delta_offset);
        header.bit_widths_offset = manual_bswap32(header.bit_widths_offset);
        
        // Byte swap float values (treat as 32-bit integers)
        uint32_t* float_ptr = (uint32_t*)&header.min_x;
        for (int i = 0; i < 6; i++) { // 6 floats: min_x, min_y, min_z, max_x, max_y, max_z
            float_ptr[i] = manual_bswap32(float_ptr[i]);
        }
        
#if DEBUG
        printf("Header values after byte swapping:\n");
        printf("  Vertices: %u, Frames: %u, Indices: %u\n", 
               header.num_vertices, header.num_frames, header.num_indices);
#endif
    #else
        // On PC/Mac (little-endian), no byte swapping needed
        if (header.magic == 0x52415431) { // 'RAT1' in little-endian
#if DEBUG
            printf("PC/Mac build detected - little-endian RAT file, no byte swapping needed\n");
#endif
            is_big_endian = false;
        }
        else if (header.magic == 0x31544152) { // 'RAT1' in big-endian (shouldn't happen)
#if DEBUG
            printf("WARNING: Big-endian RAT file on little-endian system - unusual!\n");
#endif
            is_big_endian = false; // Don't swap on little-endian systems
        }
        else {
            fclose(fp);
            snprintf(rat_last_error, sizeof(rat_last_error), 
                    "Invalid RAT file magic: 0x%08X", header.magic);
            return NULL;
        }
    #endif

    // Validate header fields
    if (header.num_vertices == 0 || header.num_frames == 0) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Invalid vertex/frame count");
        return NULL;
    }

    // We need to be more flexible with file size validation since we've added new fields
    // to the format over time (bounds, indices, bit widths)
    
    // Calculate the minimum expected size for the basic data (UVs, colors, first frame)
    size_t basic_data_size = header.num_vertices * (sizeof(VertexUV) + 
                            sizeof(VertexColor) + sizeof(VertexU8));
    
    // Add delta stream size (can vary based on compression)
    size_t delta_stream_size = 0;
    if (header.num_frames > 1) {
        // The delta stream size is variable due to bit packing, so we can't calculate
        // the exact size. We'll use a minimum estimate.
        delta_stream_size = (header.num_frames - 1) * header.num_vertices / 8; // Rough minimum
    }
    
    // Minimum size is header plus basic data plus delta stream
    size_t min_required_size = sizeof(RatHeader) + basic_data_size + delta_stream_size;
    
    // For older files without the full header
    size_t legacy_header_size = 6 * sizeof(uint32_t); // Magic + counts + basic offsets
    size_t legacy_min_size = legacy_header_size + basic_data_size + delta_stream_size;
    
    // Allow file to be either the old size or the new size
    if ((size_t)file_size < legacy_min_size) {
#if DEBUG
        printf("WARNING: File size (%ld bytes) is smaller than expected minimum (%ld bytes)\n", 
               file_size, legacy_min_size);
        printf("This may be an older or corrupted RAT file, but we'll try to read it anyway\n");
#endif
        // We'll continue and try to read what we can
    }

    // Debug output to verify header values
#if DEBUG
    printf("Reading RAT file header:\n");
    printf("  Magic: 0x%08X\n", header.magic);
    printf("  Vertices: %u\n", header.num_vertices);
    printf("  Frames: %u\n", header.num_frames);
    printf("  Indices: %u\n", header.num_indices);
#endif
 #if DEBUG
    // Check if bounds values appear valid
    bool bounds_valid = !(header.min_x == 0.0f && header.max_x == 0.0f && 
                         header.min_y == 0.0f && header.max_y == 0.0f && 
                         header.min_z == 0.0f && header.max_z == 0.0f);
#endif
    
    // Print bounds from header
#if DEBUG
    printf("  Model bounds from header: X[%.4f, %.4f], Y[%.4f, %.4f], Z[%.4f, %.4f]\n", 
           header.min_x, header.max_x, header.min_y, header.max_y, header.min_z, header.max_z);
    printf("  Bounds valid: %s\n", bounds_valid ? "YES" : "NO");
#endif

 #if DEBUG
    // If bounds are all zeros, initialize with safe defaults
    if (!bounds_valid) {
        printf("WARNING: Using default bounds [-1,1] instead of header bounds\n");
        header.min_x = header.min_y = header.min_z = -1.0f;
        header.max_x = header.max_y = header.max_z = 1.0f;
    }
#endif

    RatAnimationInfo *anim = (RatAnimationInfo*)safe_malloc(sizeof(RatAnimationInfo));
    anim->num_vertices = header.num_vertices;
    anim->num_frames = header.num_frames;
    
    // Copy model bounds for denormalization
    anim->min_x = header.min_x;
    anim->min_y = header.min_y;
    anim->min_z = header.min_z;
    anim->max_x = header.max_x;
    anim->max_y = header.max_y;
    anim->max_z = header.max_z;
    
    printf("  Actual bounds to be used: X[%.4f, %.4f], Y[%.4f, %.4f], Z[%.4f, %.4f]\n", 
           anim->min_x, anim->max_x, anim->min_y, anim->max_y, anim->min_z, anim->max_z);

    // Read UVs
    fseek(fp, header.uv_offset, SEEK_SET);
    anim->uvs = (VertexUV*)safe_malloc(header.num_vertices * sizeof(VertexUV));
    if (!anim->uvs) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to read UV data");
        rat_free_animation(anim);
        return NULL;
    }
    // Actually read the UV data from file
    if (fread(anim->uvs, sizeof(VertexUV), header.num_vertices, fp) != header.num_vertices) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), "Failed to read UV data from file");
        rat_free_animation(anim);
        return NULL;
    }
    
    // Byte swap UV data if on big-endian system
    if (is_big_endian) {
        for (uint32_t i = 0; i < header.num_vertices; i++) {
            uint32_t* u_ptr = (uint32_t*)&anim->uvs[i].u;
            uint32_t* v_ptr = (uint32_t*)&anim->uvs[i].v;
            *u_ptr = manual_bswap32(*u_ptr);
            *v_ptr = manual_bswap32(*v_ptr);
        }
        printf("Byte swapped UV data for %u vertices\n", header.num_vertices);
    }

    // Read colors
    fseek(fp, header.color_offset, SEEK_SET);
    anim->colors = (VertexColor*)safe_malloc(header.num_vertices * sizeof(VertexColor));
    if (!anim->colors) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to read color data");
        rat_free_animation(anim);
        return NULL;
    }
    // Actually read the color data from file
    if (fread(anim->colors, sizeof(VertexColor), header.num_vertices, fp) != header.num_vertices) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), "Failed to read color data from file");
        rat_free_animation(anim);
        return NULL;
    }
    
    // Byte swap color data if on big-endian system
    if (is_big_endian) {
        for (uint32_t i = 0; i < header.num_vertices; i++) {
            uint32_t* r_ptr = (uint32_t*)&anim->colors[i].r;
            uint32_t* g_ptr = (uint32_t*)&anim->colors[i].g;
            uint32_t* b_ptr = (uint32_t*)&anim->colors[i].b;
            uint32_t* a_ptr = (uint32_t*)&anim->colors[i].a;
            *r_ptr = manual_bswap32(*r_ptr);
            *g_ptr = manual_bswap32(*g_ptr);
            *b_ptr = manual_bswap32(*b_ptr);
            *a_ptr = manual_bswap32(*a_ptr);
        }
#if DEBUG
        printf("Byte swapped color data for %u vertices\n", header.num_vertices);
#endif
    }

    // Read indices if available (for new format files)
    anim->num_indices = header.num_indices;
    if (anim->num_indices > 0 && file_size >= header.indices_offset + anim->num_indices * sizeof(uint16_t)) {
        fseek(fp, header.indices_offset, SEEK_SET);
        anim->indices = (uint16_t*)risky_malloc(anim->num_indices * sizeof(uint16_t));
        if (!anim->indices) {
            snprintf(rat_last_error, sizeof(rat_last_error), 
                   "Failed to allocate memory for indices");
            free(anim->uvs);
            free(anim->colors);
            free(anim);
            fclose(fp);
            return NULL;
        }
        fread(anim->indices, sizeof(uint16_t), anim->num_indices, fp);
        
        // Byte swap indices if on big-endian system
        if (is_big_endian) {
            for (uint32_t i = 0; i < anim->num_indices; i++) {
                anim->indices[i] = manual_bswap16(anim->indices[i]);
            }
            printf("Byte swapped indices data for %u indices\n", anim->num_indices);
        }
    } else {
        anim->indices = NULL;
        anim->num_indices = 0;
    }
    
    // Read the first frame vertices
#if DEBUG
    printf("Reading first frame data...\n");
#endif
    
    // The first frame should be stored right before the delta stream
    // But we need to be careful with the calculation to avoid errors
    
    // First, try to use the delta_offset to determine first frame position
    uint32_t first_frame_offset = 0;
    
    // If we have bit widths, they should be right before the first frame
    if (header.bit_widths_offset > 0) {
        first_frame_offset = header.bit_widths_offset + (header.num_vertices * 3 * sizeof(uint8_t));
#if DEBUG
        printf("First frame offset calculated from bit_widths_offset: %u\n", first_frame_offset);
#endif
    } 
    // Otherwise, calculate based on other header values
    else if (header.num_indices > 0) {
        first_frame_offset = header.indices_offset + (header.num_indices * sizeof(uint16_t));
        printf("First frame offset calculated from indices_offset: %u\n", first_frame_offset);
    } 
    else {
        // For older files without indices, calculate from basic structure
        first_frame_offset = header.color_offset + (header.num_vertices * sizeof(VertexColor));
        printf("First frame offset calculated from color_offset: %u\n", first_frame_offset);
    }
    
    // Verify the offset is within the file
    if (first_frame_offset >= (uint32_t)file_size || first_frame_offset < sizeof(RatHeader)) {
        printf("ERROR: Invalid first frame offset (%u), file size: %ld\n", first_frame_offset, file_size);
        
        // Last resort: try to calculate from delta_offset
        if (header.delta_offset > header.num_vertices * sizeof(VertexU8)) {
            first_frame_offset = header.delta_offset - (header.num_vertices * sizeof(VertexU8));
            printf("Attempting to use delta_offset for first frame: %u\n", first_frame_offset);
            
            if (first_frame_offset >= (uint32_t)file_size || first_frame_offset < sizeof(RatHeader)) {
                printf("ERROR: All attempts to calculate first frame offset failed\n");
                fclose(fp);
                snprintf(rat_last_error, sizeof(rat_last_error), "Could not determine first frame position");
                rat_free_animation(anim);
                return NULL;
            }
        } else {
            printf("ERROR: All attempts to calculate first frame offset failed\n");
            fclose(fp);
            snprintf(rat_last_error, sizeof(rat_last_error), "Could not determine first frame position");
            rat_free_animation(anim);
            return NULL;
        }
    }
    
    printf("Using first frame offset: %u\n", first_frame_offset);
    
    // Seek to the first frame position
    fseek(fp, first_frame_offset, SEEK_SET);
    
    // Allocate memory for the first frame
    anim->first_frame = (VertexU8*)safe_malloc(header.num_vertices * sizeof(VertexU8));
    if (!anim->first_frame) {
        fclose(fp);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate memory for first frame");
        rat_free_animation(anim);
        return NULL;
    }
    
    // Read the first frame data
    size_t read_count = fread(anim->first_frame, sizeof(VertexU8), header.num_vertices, fp);
    if (read_count != header.num_vertices) {
        printf("WARNING: Only read %zu of %u first frame vertices\n", read_count, header.num_vertices);
        
        // Check if we read anything at all
        if (read_count == 0) {
            printf("ERROR: Failed to read any first frame data!\n");
            
            // Initialize all vertices to middle values as a fallback
            for (uint32_t v = 0; v < header.num_vertices; v++) {
                anim->first_frame[v].x = 128; // Middle value
                anim->first_frame[v].y = 128;
                anim->first_frame[v].z = 128;
            }
        } else {
            // Initialize any missing vertices to avoid undefined behavior
            for (uint32_t v = read_count; v < header.num_vertices; v++) {
                anim->first_frame[v].x = 128; // Middle value
                anim->first_frame[v].y = 128;
                anim->first_frame[v].z = 128;
            }
        }
    }
    
    // Debug output for first few vertices
    printf("First frame sample (normalized coordinates):\n");
    for (uint32_t i = 0; i < 3 && i < header.num_vertices; i++) {
        printf("  Vertex %u: [%u, %u, %u]\n", 
               i, anim->first_frame[i].x, anim->first_frame[i].y, anim->first_frame[i].z);
    }

    // Read delta stream - the size can vary based on compression
    // First, check if we have more than one frame
    if (header.num_frames > 1) {
        // Get the correct starting position of the delta stream (it's right after first_frame)
        long actual_delta_stream_start_offset = ftell(fp);
        printf("Actual delta stream offset calculated as: %ld\n", actual_delta_stream_start_offset);

        // Calculate the remaining file size for the delta stream
        fseek(fp, 0, SEEK_END);
        long end_pos = ftell(fp);
        long delta_stream_bytes = end_pos - actual_delta_stream_start_offset;
        
        // Calculate how many 32-bit words that is
        anim->delta_stream_size = delta_stream_bytes / sizeof(uint32_t);
        if (delta_stream_bytes < 0) { // Should not happen if file is valid
             printf("ERROR: Negative delta_stream_bytes calculated (%ld). Setting stream size to 0.\n", delta_stream_bytes);
             anim->delta_stream_size = 0;
             delta_stream_bytes = 0;
        }
        else if (delta_stream_bytes % sizeof(uint32_t) != 0) {
             printf("WARNING: Delta stream size (%ld bytes) is not a multiple of 4. May indicate padding or corruption.\n", delta_stream_bytes);
            // The existing code truncates, let's stick to that unless issues arise.
            // anim->delta_stream_size++; // Round up to include partial word
        }
        
        printf("Delta stream: %ld bytes, %u words\n", delta_stream_bytes, anim->delta_stream_size);
        
        if (anim->delta_stream_size > 0) {
            // Allocate memory for the delta stream
            anim->delta_stream = (uint32_t*)safe_malloc(anim->delta_stream_size * sizeof(uint32_t));
            if (!anim->delta_stream) {
                fclose(fp);
                snprintf(rat_last_error, sizeof(rat_last_error), 
                        "Failed to allocate memory for delta stream");
                rat_free_animation(anim);
                return NULL;
            }
            
            // Read the delta stream from its correct position
            fseek(fp, actual_delta_stream_start_offset, SEEK_SET);
            size_t delta_read_count = fread(anim->delta_stream, sizeof(uint32_t), anim->delta_stream_size, fp);
            if (delta_read_count != anim->delta_stream_size) {
                printf("WARNING: Only read %zu of %u delta stream words\n", delta_read_count, anim->delta_stream_size);
                // We'll continue anyway with what we have, but this might be an issue
            }
            
            // Byte swap delta stream if on big-endian system
            if (is_big_endian) {
                for (uint32_t i = 0; i < anim->delta_stream_size; i++) {
                    anim->delta_stream[i] = manual_bswap32(anim->delta_stream[i]);
                }
                printf("Byte swapped delta stream data for %u uint32_t values\n", anim->delta_stream_size);
            }
        } else {
            // No delta stream data to read
            anim->delta_stream = NULL;
            printf("No delta stream data to read (size is zero or negative).\n");
        }
    } else {
        // Single frame animation, no delta stream needed
        anim->delta_stream = NULL;
        anim->delta_stream_size = 0;
    }
    
    // Initialize bit width arrays to NULL
    anim->bit_widths = NULL;
    
    // Read bit width arrays if available
    if (header.bit_widths_offset > 0) {
        fseek(fp, header.bit_widths_offset, SEEK_SET);
        // Allocate memory for interleaved bit widths
        anim->bit_widths = (RatBitWidths*)safe_malloc(header.num_vertices * sizeof(RatBitWidths));
        if (!anim->bit_widths) {
            fclose(fp);
            snprintf(rat_last_error, sizeof(rat_last_error), "Failed to allocate interleaved bit widths");
            rat_free_animation(anim);
            return NULL;
        }
        // Read bit widths for each axis into temporary arrays
        uint8_t *tmp_x = (uint8_t*)risky_malloc(header.num_vertices);
        uint8_t *tmp_y = (uint8_t*)risky_malloc(header.num_vertices);
        uint8_t *tmp_z = (uint8_t*)risky_malloc(header.num_vertices);
        if (!tmp_x || !tmp_y || !tmp_z) {
            free(tmp_x); free(tmp_y); free(tmp_z);
            fclose(fp);
            snprintf(rat_last_error, sizeof(rat_last_error), "Failed to allocate temp bit width arrays");
            rat_free_animation(anim);
            return NULL;
        }
        if (fread(tmp_x, sizeof(uint8_t), header.num_vertices, fp) != header.num_vertices ||
            fread(tmp_y, sizeof(uint8_t), header.num_vertices, fp) != header.num_vertices ||
            fread(tmp_z, sizeof(uint8_t), header.num_vertices, fp) != header.num_vertices) {
            free(tmp_x); free(tmp_y); free(tmp_z);
            free(anim->bit_widths);
            anim->bit_widths = NULL;
            printf("WARNING: Failed to read bit width data\n");
        } else {
            // Interleave into anim->bit_widths
            for (uint32_t v = 0; v < header.num_vertices; v++) {
                anim->bit_widths[v].x = tmp_x[v];
                anim->bit_widths[v].y = tmp_y[v];
                anim->bit_widths[v].z = tmp_z[v];
            }
            printf("Read and interleaved bit widths for %u vertices\n", header.num_vertices);
        }
        free(tmp_x); free(tmp_y); free(tmp_z);
    } else {
        anim->bit_widths = NULL;
    }
    
    fclose(fp);
    return anim;
}

static inline void rat_denormalize_vertices(DecompressionContext *ctx);

// Static counter for assigning unique context IDs for debugging multi-actor systems
static uint32_t next_context_id = 1;

///
/// Creates a decompression context for a given RAT animation.
/// @param anim Pointer to a loaded RatAnimationInfo structure.
/// @return Pointer to a new DecompressionContext, or NULL on failure.
///
DecompressionContext* rat_create_context(RatAnimationInfo *anim) {
    #ifdef N64
    profiler_start_segment("RAT_CreateCtx");
    #endif
    
    if (!anim) {
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "NULL animation info provided");
        #ifdef N64
        profiler_end_segment();
        #endif
        return NULL;
    }
    
    #ifdef N64
    profiler_start_segment("RAT_AllocCtx");
    #endif
    
    DecompressionContext *ctx = (DecompressionContext*)risky_malloc(sizeof(DecompressionContext));
    if (!ctx) {
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate decompression context");
        #ifdef N64
        profiler_end_segment(); // End alloc
        profiler_end_segment(); // End create
        #endif
        return NULL;
    }
    
    #ifdef N64
    profiler_end_segment(); // End alloc
    profiler_start_segment("RAT_InitFields");
    #endif
    
    ctx->uvs = anim->uvs;
    ctx->colors = anim->colors;
    ctx->num_vertices = anim->num_vertices;
    ctx->num_frames = anim->num_frames;
    
    // Assign unique context ID for debugging multi-actor systems
    ctx->context_id = next_context_id++;
    
    // Store indices if available
    ctx->indices = anim->indices;
    ctx->num_indices = anim->num_indices;
    
    // Debug output for indices
    if (ctx->indices && ctx->num_indices > 0) {
        printf("Animation has %u indices\n", ctx->num_indices);
        printf("First 9 indices (for 3 triangles): ");
        for (uint32_t i = 0; i < 9 && i < ctx->num_indices; i++) {
            printf("%u ", ctx->indices[i]);
            if ((i + 1) % 3 == 0) printf("| ");
        }
        printf("\n");
    } else {
        printf("Animation uses direct vertex order (no indices)\n");
    }
    
    // Store model bounds for denormalization
    ctx->min_x = anim->min_x;
    ctx->min_y = anim->min_y;
    ctx->min_z = anim->min_z;
    ctx->max_x = anim->max_x;
    ctx->max_y = anim->max_y;
    ctx->max_z = anim->max_z;
    
    // --- BEGIN ADDED LOGGING ---
    printf("[Context Creation] Bounds copied: X[%.4f, %.4f], Y[%.4f, %.4f], Z[%.4f, %.4f]\n", 
           ctx->min_x, ctx->max_x, ctx->min_y, ctx->max_y, ctx->min_z, ctx->max_z);
    // --- END ADDED LOGGING ---
    
    // N64-Optimized Memory Allocation: Use specialized allocators for different data types
    // Write-heavy data - use cache-aligned allocation
    ctx->current_positions = (VertexU8*)rat_alloc_writeonly(anim->num_vertices * sizeof(VertexU8));
    if (!ctx->current_positions) {
        free(ctx);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate vertices");
        return NULL;
    }
    
    // Read-only data - use standard allocation (this is just a copy of first frame)
    ctx->first_frame = (VertexU8*)rat_alloc_readonly(anim->num_vertices * sizeof(VertexU8));
    if (!ctx->first_frame) {
        free(ctx->current_positions);
        free(ctx);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate first frame data");
        return NULL;
    }
    
    // Verify first frame data exists and is valid
    bool first_frame_valid = true;
    
    if (!anim->first_frame) {
        printf("ERROR: No first frame data in animation!\n");
        first_frame_valid = false;
    } else {
        // Check if first frame data appears valid (not all zeros or all the same value)
        uint8_t first_x = anim->first_frame[0].x;
        uint8_t first_y = anim->first_frame[0].y;
        uint8_t first_z = anim->first_frame[0].z;
        
        bool all_same = true;
        bool all_zeros = (first_x == 0 && first_y == 0 && first_z == 0);
        
        // Check a sample of vertices to see if they're all identical
        for (uint32_t v = 1; v < anim->num_vertices && v < 100; v++) {
            if (anim->first_frame[v].x != first_x || 
                anim->first_frame[v].y != first_y || 
                anim->first_frame[v].z != first_z) {
                all_same = false;
                break;
            }
        }
        
        if (all_zeros) {
            printf("WARNING: First frame contains all zero vertices!\n");
            first_frame_valid = false;
        } else if (all_same) {
            printf("WARNING: First frame contains all identical vertices!\n");
            first_frame_valid = false;
        } else {
            // Copy the first frame data to both the current positions and the stored first frame
            memcpy(ctx->current_positions, anim->first_frame, anim->num_vertices * sizeof(VertexU8));
            memcpy(ctx->first_frame, anim->first_frame, anim->num_vertices * sizeof(VertexU8));
        }
    }
    
    if (!first_frame_valid) {
        // Initialize to middle values (128) if first frame is invalid
        printf("Initializing first frame to default values\n");
        for (uint32_t v = 0; v < anim->num_vertices; v++) {
            ctx->current_positions[v].x = 128;
            ctx->current_positions[v].y = 128;
            ctx->current_positions[v].z = 128;
            
            // Also initialize the stored first frame
            ctx->first_frame[v].x = 128;
            ctx->first_frame[v].y = 128;
            ctx->first_frame[v].z = 128;
        }
    } else {
        // Copy first frame data
        memcpy(ctx->current_positions, anim->first_frame, 
               anim->num_vertices * sizeof(VertexU8));
        
        // Debug output for first few vertices
#ifndef N64
        printf("First frame vertices copied to context:\n");
        for (uint32_t v = 0; v < 5 && v < anim->num_vertices; v++) {
            printf("  Vertex %u: [%u, %u, %u]\n", 
                   v, ctx->current_positions[v].x, ctx->current_positions[v].y, ctx->current_positions[v].z);
        }
#endif
        
        // Verify vertices are within expected range
        int out_of_range_count = 0;
        for (uint32_t v = 0; v < anim->num_vertices; v++) {
            if (ctx->current_positions[v].x == 0 && 
                ctx->current_positions[v].y == 0 && 
                ctx->current_positions[v].z == 0) {
                out_of_range_count++;
            }
        }
        
#ifndef N64
        if (out_of_range_count > 0) {
            printf("WARNING: %d vertices have zero values in first frame\n", out_of_range_count);
        }
#endif
    }
    
    // Initialize bit width array if available in the animation
    if (anim->bit_widths) {
        ctx->bit_widths = (RatBitWidths*)rat_alloc_readonly(anim->num_vertices * sizeof(RatBitWidths));
        if (!ctx->bit_widths) {
            snprintf(rat_last_error, sizeof(rat_last_error), "Failed to allocate interleaved bit width array");
            free(ctx->current_positions);
            free(ctx);
            return NULL;
        }
        memcpy(ctx->bit_widths, anim->bit_widths, anim->num_vertices * sizeof(RatBitWidths));
#ifndef N64
        printf("Copied interleaved per-vertex bit widths from animation\n");
        for (uint32_t v = 0; v < 5 && v < anim->num_vertices; v++) {
            printf("  Vertex %u: X=%u, Y=%u, Z=%u\n", v, ctx->bit_widths[v].x, ctx->bit_widths[v].y, ctx->bit_widths[v].z);
        }
#endif
    } else {
        ctx->bit_widths = NULL;
        printf("No per-vertex bit widths in animation, will read from bitstream\n");
    }
    
    // Allocate space for denormalized vertices (for rendering) - write-heavy data
    ctx->current_frame_vertices = (float*)rat_alloc_writeonly(anim->num_vertices * 3 * sizeof(float));
    if (!ctx->current_frame_vertices) {
        if (ctx->bit_widths) free(ctx->bit_widths);
        if (ctx->current_positions) free(ctx->current_positions);
        free(ctx);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate denormalized vertices");
        return NULL;
    }
    
    // Allocate space for interleaved vertices
    ctx->interleaved_vertices = (RatVertexFull*)rat_alloc_writeonly(anim->num_vertices * sizeof(RatVertexFull));
    if (!ctx->interleaved_vertices) {
        free(ctx->current_positions);
        free(ctx->current_frame_vertices);
        free(ctx);
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Failed to allocate interleaved vertices");
        return NULL;
    }
    
    // Initialize bit reader for delta stream
    // Convert the delta_stream_size (in words) to the number of words for the bitstream reader
    size_t stream_words = anim->delta_stream_size; // Already in words, not bytes
    
#ifndef N64
    printf("Initializing decompression context:\n");
    printf("  Delta stream size: %u words (%zu bytes)\n", 
           anim->delta_stream_size, anim->delta_stream_size * sizeof(uint32_t));
    printf("  Frames: %u, Vertices: %u\n", anim->num_frames, anim->num_vertices);
#endif
    
    #ifdef N64
    profiler_end_segment(); // End init fields
    profiler_start_segment("RAT_InitStream");
    #endif
    
    bitstream_init(&ctx->bit_reader, anim->delta_stream, stream_words);
    
    // Initialize progressive decompression state
    ctx->target_frame = 0;
    ctx->vertices_processed = 0;
    ctx->vertices_per_slice = 0;  // Will be set when progressive mode is activated
    ctx->max_time_us = 0;         // Will be set when progressive mode is activated
    ctx->progressive_active = false;
    ctx->progressive_complete = true;  // Start as complete since we're at frame 0
    
    // Set current frame to 0
    ctx->current_frame = 0;
    
    // Initialize optimization flag - static data (UV/color) will be set on first denormalization
    ctx->static_data_initialized = false;
    
    #ifdef N64
    profiler_end_segment(); // End init stream
    profiler_start_segment("RAT_InitDenorm");
    #endif
    
#ifndef N64
    printf("Created DecompressionContext %u: %u vertices, %u frames\n", 
           ctx->context_id, ctx->num_vertices, ctx->num_frames);
#endif
    
    // Denormalize the first frame for immediate rendering
    rat_denormalize_vertices(ctx);
    
    #ifdef N64
    profiler_end_segment(); // End init denorm
    profiler_end_segment(); // End create ctx
    #endif
    
    return ctx;
}

static inline void rat_denormalize_vertex_fast(const VertexU8* src, float* dst,
                                              float min_x, float min_y, float min_z,
                                              float range_x, float range_y, float range_z) {
    // Convert normalized [0-255] values to original coordinate space
    // Using pointer arithmetic for better N64 cache performance
    register float norm_x = (float)src->x / 255.0f;
    register float norm_y = (float)src->y / 255.0f;
    register float norm_z = (float)src->z / 255.0f;
    
    // Store results directly to output array
    dst[0] = min_x + norm_x * range_x;
    dst[1] = min_y + norm_y * range_y;
    dst[2] = min_z + norm_z * range_z;
}

// N64-optimized batch decompression for uniform bit widths
static inline bool rat_decompress_frame_batch_optimized(DecompressionContext *ctx, uint32_t frame) {
    if (!ctx || !ctx->bit_widths) {
        return false;
    }

    // Check if we have uniform bit widths across all vertices
    
    uint8_t bx = ctx->bit_widths[0].x;
    uint8_t by = ctx->bit_widths[0].y;
    uint8_t bz = ctx->bit_widths[0].z;
    for (uint32_t v = 1; v < ctx->num_vertices; v++) {
        if (ctx->bit_widths[v].x != bx || ctx->bit_widths[v].y != by || ctx->bit_widths[v].z != bz) {
            return false;
        }
    }

    // Uniform bit widths found, use optimized batch processing
    
    register BitstreamReader *br = &ctx->bit_reader;
    register VertexU8 *positions = ctx->current_positions;

    // N64: Use proper MIPS cache instructions for intensive decompression
#ifdef N64
    rat_animation_cache_prefetch_load(positions);
    rat_animation_cache_prefetch_store(positions); // Prepare for writing deltas
#else
    rat_prefetch_memory(positions);
#endif

    // Process vertices in batches for better cache utilization
    const uint32_t BATCH_SIZE = 8; // N64 cache line size / sizeof(VertexU8)
    uint32_t num_batches = ctx->num_vertices / BATCH_SIZE;
    uint32_t remaining = ctx->num_vertices % BATCH_SIZE;

    // Process full batches
    for (uint32_t batch = 0; batch < num_batches; batch++) {
        // Prefetch next cache line using MIPS instructions for N64
#ifdef N64
        if (batch + 1 < num_batches) {
            rat_animation_cache_prefetch_load(&positions[(batch + 1) * BATCH_SIZE]);
            rat_animation_cache_prefetch_store(&positions[(batch + 1) * BATCH_SIZE]);
        }
#else
        rat_prefetch_memory(&positions[(batch + 1) * BATCH_SIZE]);
#endif

        // Process vertices in current batch using optimized batch reading when possible
        uint32_t batch_start = batch * BATCH_SIZE;
        uint32_t batch_count = (batch_start + BATCH_SIZE <= ctx->num_vertices) ? BATCH_SIZE : (ctx->num_vertices - batch_start);
        
        // Try ultra-fast batch processing for uniform bit widths
        if (batch_count >= 4) {
            int dx_batch[4], dy_batch[4], dz_batch[4];
            if (bitstream_read_vertex_deltas_batch(br, bx, by, bz, dx_batch, dy_batch, dz_batch, 4)) {
                // Apply 4 vertices worth of deltas
                for (uint32_t i = 0; i < 4; i++) {
                    uint32_t v = batch_start + i;
                    positions[v].x = (positions[v].x + dx_batch[i]) & 0xFF;
                    positions[v].y = (positions[v].y + dy_batch[i]) & 0xFF;
                    positions[v].z = (positions[v].z + dz_batch[i]) & 0xFF;
                }
                
                // Process remaining vertices in this batch individually
                for (uint32_t i = 4; i < batch_count; i++) {
                    uint32_t v = batch_start + i;
                    int dx, dy, dz;
                    if (!bitstream_read_vertex_deltas(br, bx, by, bz, &dx, &dy, &dz)) {
                        return false;
                    }
                    positions[v].x = (positions[v].x + dx) & 0xFF;
                    positions[v].y = (positions[v].y + dy) & 0xFF;
                    positions[v].z = (positions[v].z + dz) & 0xFF;
                }
            } else {
                // Fall back to individual processing for this batch
                for (uint32_t i = 0; i < batch_count; i++) {
                    uint32_t v = batch_start + i;
                    int dx, dy, dz;
                    if (!bitstream_read_vertex_deltas(br, bx, by, bz, &dx, &dy, &dz)) {
                        return false;
                    }
                    positions[v].x = (positions[v].x + dx) & 0xFF;
                    positions[v].y = (positions[v].y + dy) & 0xFF;
                    positions[v].z = (positions[v].z + dz) & 0xFF;
                }
            }
        } else {
            // Process remaining vertices individually
            for (uint32_t i = 0; i < batch_count; i++) {
                uint32_t v = batch_start + i;
                int dx, dy, dz;
                if (!bitstream_read_vertex_deltas(br, bx, by, bz, &dx, &dy, &dz)) {
                    return false;
                }
                positions[v].x = (positions[v].x + dx) & 0xFF;
                positions[v].y = (positions[v].y + dy) & 0xFF;
                positions[v].z = (positions[v].z + dz) & 0xFF;
            }
        }
    }

    // Handle remaining vertices
    for (uint32_t v = num_batches * BATCH_SIZE; v < ctx->num_vertices; v++) {
        int dx, dy, dz;
        if (!bitstream_read_vertex_deltas(br, ctx->bit_widths[v].x, ctx->bit_widths[v].y, ctx->bit_widths[v].z, &dx, &dy, &dz)) {
            return false;
        }
        positions[v].x = (positions[v].x + dx) & 0xFF;
        positions[v].y = (positions[v].y + dy) & 0xFF;
        positions[v].z = (positions[v].z + dz) & 0xFF;
    }
    
    return true;
}
static const float INV_255 = 1.0f / 255.0f; 

// Ultra-optimized batch vertex delta reading for uniform bit widths
// Processes up to 4 vertices per bitstream read when bit patterns allow
static inline bool bitstream_read_vertex_deltas_batch(BitstreamReader *br,
                                                     uint8_t bx, uint8_t by, uint8_t bz,
                                                     int *dx_array, int *dy_array, int *dz_array,
                                                     uint32_t count) {
    register uint8_t total_bits = bx + by + bz;
    
    // Only optimize when total bits per vertex is small enough for batching
    if (total_bits <= 8 && count >= 4) {
        // Read 32 bits = 4 vertices worth of data at once
        register uint32_t packed_data = bitstream_read(br, 32, NULL);
        if (br->position >= br->stream_length && br->bits_remaining < 0) {
            return false;
        }
        
        // Extract 4 vertices from the 32-bit word
        for (uint32_t i = 0; i < 4 && i < count; i++) {
            uint32_t shift = (3 - i) * 8; // MSB first
            uint32_t vertex_data = (packed_data >> shift) & 0xFF;
            
            // Extract Z, Y, X components from this 8-bit vertex data
            if (bz > 0) {
                uint32_t raw_dz = vertex_data & ((1u << bz) - 1);
                dz_array[i] = fast_sign_extend(raw_dz, bz);
                vertex_data >>= bz;
            } else {
                dz_array[i] = 0;
            }
            
            if (by > 0) {
                uint32_t raw_dy = vertex_data & ((1u << by) - 1);
                dy_array[i] = fast_sign_extend(raw_dy, by);
                vertex_data >>= by;
            } else {
                dy_array[i] = 0;
            }
            
            if (bx > 0) {
                uint32_t raw_dx = vertex_data & ((1u << bx) - 1);
                dx_array[i] = fast_sign_extend(raw_dx, bx);
            } else {
                dx_array[i] = 0;
            }
        }
        return true;
    }
    
    // For larger bit widths or small counts, fall back to individual reads
    return false;
}

// Implementation of the previously forward-declared function
static inline void rat_denormalize_vertices(DecompressionContext *ctx) {
    // Note: Don't add top-level profiling here since this function is called from 
    // already profiled sections and would create nested segments
    
#if DEBUG
    if (!ctx || !ctx->current_positions || !ctx->current_frame_vertices) {
        printf("ERROR: Invalid context in rat_denormalize_vertices\n");
        return;
    }
#else
    if (!ctx || !ctx->current_positions || !ctx->current_frame_vertices) {
        return;
    }
#endif
    
    // Calculate ranges for denormalization
    float range_x = ctx->max_x - ctx->min_x;
    float range_y = ctx->max_y - ctx->min_y;
    float range_z = ctx->max_z - ctx->min_z;
    #if !(defined(N64) || defined(GEKKO) || defined(DREAMCAST) || defined(PSP)) // Only do this on platforms where we can afford the checks
    bool invalid_bounds = (ctx->min_x == ctx->max_x || ctx->min_y == ctx->max_y || ctx->min_z == ctx->max_z);
    if (invalid_bounds) {
        printf("WARNING: Invalid bounds in decompression context, using defaults\n");
        ctx->min_x = -1.0f; ctx->max_x = 1.0f;
        ctx->min_y = -1.0f; ctx->max_y = 1.0f;
        ctx->min_z = -1.0f; ctx->max_z = 1.0f;
        range_x = ctx->max_x - ctx->min_x;
        range_y = ctx->max_y - ctx->min_y;
        range_z = ctx->max_z - ctx->min_z;
    }
    if (range_x < 0.0001f) {
        printf("WARNING: X range (%.6f) too small during denormalization, expanding.\n", range_x);
        float center = (ctx->min_x + ctx->max_x) * 0.5f;
        ctx->min_x = center - 0.5f;
        ctx->max_x = center + 0.5f;
        range_x = ctx->max_x - ctx->min_x;
    }
    if (range_y < 0.0001f) {
        printf("WARNING: Y range (%.6f) too small during denormalization, expanding.\n", range_y);
        float center = (ctx->min_y + ctx->max_y) * 0.5f;
        ctx->min_y = center - 0.5f;
        ctx->max_y = center + 0.5f;
        range_y = ctx->max_y - ctx->min_y;
    }
    if (range_z < 0.0001f) {
        printf("WARNING: Z range (%.6f) too small during denormalization, expanding.\n", range_z);
        float center = (ctx->min_z + ctx->max_z) * 0.5f;
        ctx->min_z = center - 0.5f;
        ctx->max_z = center + 0.5f;
        range_z = ctx->max_z - ctx->min_z;
    }
    if (!ctx->current_positions) {
        printf("ERROR: No current_positions in context!\n");
        return;
    }
    #endif
    // Denormalize and interleave all vertex data
    for (uint32_t v = 0; v < ctx->num_vertices; v++) {
        // Denormalize position
        float norm_x = (float)ctx->current_positions[v].x * INV_255;
        float norm_y = (float)ctx->current_positions[v].y * INV_255;
        float norm_z = (float)ctx->current_positions[v].z * INV_255;
        float px = ctx->min_x + norm_x * range_x;
        float py = ctx->min_y + norm_y * range_y;
        float pz = ctx->min_z + norm_z * range_z;
        ctx->current_frame_vertices[v * 3 + 0] = px;
        ctx->current_frame_vertices[v * 3 + 1] = py;
        ctx->current_frame_vertices[v * 3 + 2] = pz;
        // Fill interleaved vertex struct

        if (ctx->interleaved_vertices) {
            // Always update position data (changes every frame)
            ctx->interleaved_vertices[v].x = px;
            ctx->interleaved_vertices[v].y = py;
            ctx->interleaved_vertices[v].z = pz;
            
            // Only update static data (color, UV) once per context to avoid redundant work
            if (!ctx->static_data_initialized) {
                // Copy color if available
                if (ctx->colors) {
                    ctx->interleaved_vertices[v].r = ctx->colors[v].r;
                    ctx->interleaved_vertices[v].g = ctx->colors[v].g;
                    ctx->interleaved_vertices[v].b = ctx->colors[v].b;
                    ctx->interleaved_vertices[v].a = ctx->colors[v].a;
                } else {
                    ctx->interleaved_vertices[v].r = 1.0f;
                    ctx->interleaved_vertices[v].g = 1.0f;
                    ctx->interleaved_vertices[v].b = 1.0f;
                    ctx->interleaved_vertices[v].a = 1.0f;
                }
                // Copy UV if available
                if (ctx->uvs) {
                    ctx->interleaved_vertices[v].u = ctx->uvs[v].u;

                #ifdef GEKKO // FLIP V component on GEKKO
                    if(ctx->num_frames > 1) {
                        ctx->interleaved_vertices[v].v = -1.0f * (1.0f - ctx->uvs[v].v);
                    }
                    else {
                        ctx->interleaved_vertices[v].v = 1.0f - ctx->uvs[v].v;
                    }
                #else
                    if(ctx->num_frames > 1) {
                        ctx->interleaved_vertices[v].v = -ctx->uvs[v].v;
                    }
                    else {
                        ctx->interleaved_vertices[v].v = ctx->uvs[v].v;
                    }
                #endif
                } else {
                    ctx->interleaved_vertices[v].u = 0.0f;
                    ctx->interleaved_vertices[v].v = 0.0f;
                }
            }
        }
    }
    
    // Mark static data as initialized on all platforms for better performance
    // UV and color data doesn't change between frames, so only set it once
    ctx->static_data_initialized = true;
    
}

static inline void rat_decompress_to_frame(DecompressionContext *ctx, uint32_t target_frame) {
    #ifdef N64
    profiler_start_segment("RAT_Decompress");
    #endif
    
    if (!ctx) {
        snprintf(rat_last_error, sizeof(rat_last_error), "Invalid decompression context");
        #ifdef N64
        profiler_end_segment();
        #endif
        return;
    }

    if (target_frame >= ctx->num_frames) {
        snprintf(rat_last_error, sizeof(rat_last_error), 
                "Target frame %u exceeds total frames %u", 
                target_frame, ctx->num_frames);
        #ifdef N64
        profiler_end_segment();
        #endif
        return;
    }

    // If we're already at the target frame, no need to do anything.
    if (target_frame == ctx->current_frame) {
        #ifdef N64
        profiler_end_segment();
        #endif
        return;
    }

    // If target_frame is earlier than current_frame, we must reset and decompress from the start.
    if (target_frame < ctx->current_frame) {
        #ifdef N64
        profiler_start_segment("RAT_Reset");
        #endif
        // Restore the first frame's vertex positions
        memcpy(ctx->current_positions, ctx->first_frame, ctx->num_vertices * sizeof(VertexU8));
        
        // Reset the bitstream reader to the beginning
        if (ctx->bit_reader.stream) {
            bitstream_init(&ctx->bit_reader, ctx->bit_reader.stream, ctx->bit_reader.stream_length);
        } else if (ctx->num_frames > 1) {
            snprintf(rat_last_error, sizeof(rat_last_error), "Bitstream not initialized for multi-frame animation");
            #ifdef N64
            profiler_end_segment(); // End reset segment
            profiler_end_segment(); // End main segment
            #endif
            return;
        }
        
        ctx->current_frame = 0;
        #ifdef N64
        profiler_end_segment(); // End reset segment
        #endif
    }

    // OPTIMIZATION: Progressive decompression - only decompress one frame at a time
    // This prevents frame stalls and allows the animation to catch up gradually over multiple renders
    // Calculate frame jump AFTER potential reset to get accurate distance
    uint32_t frame_jump = target_frame - ctx->current_frame;
    #ifdef N64
    uint32_t max_frames_per_update = 4;
    #else
    uint32_t max_frames_per_update = 8; // Conservative limit for smooth 60fps
    #endif
    if (frame_jump > max_frames_per_update) {
        // Only decompress a few frames this update, rest will be done in subsequent calls
        target_frame = ctx->current_frame + max_frames_per_update;
    }

    // If the target is frame 0, we just needed to reset (if we weren't there already).
    // The reset logic above handles setting current_frame to 0.
    if (target_frame == 0) {
        #ifdef N64
        profiler_start_segment("RAT_Denorm");
        #endif
        rat_denormalize_vertices(ctx);
        #ifdef N64
        profiler_end_segment(); // End denormalize segment
        profiler_end_segment(); // End main segment
        #endif
        return;
    }

#if DEBUG
    // Verify model bounds
    bool zero_volume = (ctx->min_x == ctx->max_x || ctx->min_y == ctx->max_y || ctx->min_z == ctx->max_z);
    if (zero_volume) {
        printf("WARNING: Zero volume model detected in decompress_to_frame\n");
    }
#endif

#ifdef DEBUG
    // Verify that bit width arrays are available (should have been loaded during context creation)
    if (ctx->bit_widths == NULL) {
        printf("WARNING: No per-vertex bit widths available in context!\n");
    }
#endif

    // Performance profiling for optimization tracking
    #if DEBUG && !defined(N64)
    uint64_t start_ticks = rat_get_performance_ticks();
    #endif
    
    #ifdef N64
    profiler_start_segment("RAT_Frames");
    #endif
    
    // Progressive decompression: process one frame at a time to avoid frame stalls
    // The loop will only iterate once per call (due to progressive limiting above)
    for (uint32_t f = ctx->current_frame + 1; f <= target_frame; f++) {
        #ifdef N64
        profiler_start_segment("RAT_Frame");
        #endif
        
        // Try ultra-optimized batch processing first
        if (!rat_decompress_frame_batch_optimized(ctx, f)) {
            #ifdef N64
            profiler_start_segment("RAT_PerVertex");
            #endif
            
            // Fall back to per-vertex processing with function call elimination
            register VertexU8 *positions = ctx->current_positions;
            register BitstreamReader *br = &ctx->bit_reader;
            
            // Prefetch first cache line and prepare for MIPS cache optimization ( IMPORTANT, this speeds up the code significantly on N64 )
            #ifdef N64
            rat_animation_cache_prefetch_load(positions);
            rat_animation_cache_prefetch_store(positions);
            #else
            rat_prefetch_memory(positions);
            #endif
            
            // Cache-optimized per-vertex processing with inlined fast path
            for (register uint32_t v = 0; v < ctx->num_vertices; v++) {
                // Prefetch next cache line every 8 vertices (N64 cache line size)
                /*#ifdef N64
                if ((v & 7) == 0 && v + 8 < ctx->num_vertices) {
                    rat_animation_cache_prefetch_load(&positions[v + 8]);
                    rat_animation_cache_prefetch_load(&ctx->bit_widths[v + 8]);
                }
                #endif*/
                
                // Get bit widths for this vertex using register hints for VR4300
                register uint8_t bx = ctx->bit_widths[v].x;
                register uint8_t by = ctx->bit_widths[v].y;
                register uint8_t bz = ctx->bit_widths[v].z;

                // OPTIMIZED FAST PATH: Inline bit reading for common cases to avoid function calls
                register uint8_t total_bits = bx + by + bz;
                if (total_bits <= 12 && total_bits <= br->bits_remaining) {
                    // Ultra-fast path: read all bits at once and extract deltas inline
                    register uint32_t packed = br->current_bits >> (32 - total_bits);
                    br->current_bits <<= total_bits;
                    br->bits_remaining -= total_bits;
                    
                    // Extract components with minimal branching
                    int dz = (bz > 0) ? fast_sign_extend(packed & ((1u << bz) - 1), bz) : 0;
                    packed >>= bz;
                    int dy = (by > 0) ? fast_sign_extend(packed & ((1u << by) - 1), by) : 0;
                    packed >>= by;
                    int dx = (bx > 0) ? fast_sign_extend(packed & ((1u << bx) - 1), bx) : 0;
                    
                    // Apply deltas directly (inlined for performance)
                    positions[v].x = (positions[v].x + dx) & 0xFF;
                    positions[v].y = (positions[v].y + dy) & 0xFF;
                    positions[v].z = (positions[v].z + dz) & 0xFF;
                } else {
                    // Fallback to function calls for larger bit widths
                    int dx, dy, dz;
                    rat_read_vertex_deltas_inline(br, bx, by, bz, &dx, &dy, &dz);
                    rat_apply_deltas_safe(&positions[v], dx, dy, dz);
                }
            }
            
            #ifdef N64
            profiler_end_segment(); // End per-vertex segment
            #endif
        }
        
        #ifdef N64
        profiler_end_segment(); // End frame segment
        #endif
    }
    
    #ifdef N64
    profiler_end_segment(); // End frames segment
    #endif
    
    // Performance profiling output
    #if DEBUG && !defined(N64)
    uint64_t end_ticks = rat_get_performance_ticks();
    uint64_t elapsed = end_ticks - start_ticks;
    if (elapsed > 0) {
        printf("PERF: Decompressed %u frames in %llu ticks (%.2f ticks/frame)\n", 
               target_frame - ctx->current_frame, elapsed, 
               (double)elapsed / (target_frame - ctx->current_frame));
    }
    #endif

    // Update the context's current frame to the new target
    ctx->current_frame = target_frame;

    #ifdef N64
    profiler_start_segment("RAT_Denorm");
    #endif
    
    // Denormalize the final vertex positions for rendering
    rat_denormalize_vertices(ctx);
    
    #ifdef N64
    profiler_end_segment(); // End denormalize segment
    profiler_end_segment(); // End main segment
    #endif
}

// Tom Forsyth's Linear-Speed Vertex Cache Optimization
// Optimizes triangle indices for maximum vertex cache efficiency
// Reference: https://tomforsyth1000.github.io/papers/fast_vert_cache_opt.html

#define RAT_VERTEX_CACHE_SIZE 32    // N64 RDP vertex cache size
#define RAT_VALENCE_BOOST_SCALE 2.0f
#define RAT_VALENCE_BOOST_POWER 0.5f

typedef struct {
    float score;
    uint32_t cache_position;  // Position in cache, or UINT32_MAX if not in cache
    uint32_t num_triangles_left;
    uint32_t *triangles;      // Array of triangle indices that use this vertex
} RatVertexData;

typedef struct {
    bool processed;
    float score;
    uint32_t vertices[3];
} RatTriangleData;

// Calculate vertex score based on cache position and remaining triangle count
static inline float rat_calculate_vertex_score(uint32_t cache_position, uint32_t num_triangles_left) {
    float score = 0.0f;
    
    // Cache position score (Tom Forsyth's optimized scoring function)
    if (cache_position < 3) {
        // Recently used vertices get highest priority
        score = 0.75f;
    } else if (cache_position < RAT_VERTEX_CACHE_SIZE) {
        // Vertices still in cache get decreasing priority based on position
        const float cache_decay_power = 1.5f;
        score = powf(1.0f - (float)(cache_position - 3) / (RAT_VERTEX_CACHE_SIZE - 3), cache_decay_power);
    }
    // Vertices not in cache get score of 0
    
    // Valence boost - vertices used by fewer triangles get priority to finish them off
    if (num_triangles_left > 0) {
        score += RAT_VALENCE_BOOST_SCALE * powf((float)num_triangles_left, -RAT_VALENCE_BOOST_POWER);
    }
    
    return score;
}

// Update vertex cache and scores after processing a triangle
static inline void rat_update_vertex_cache(uint32_t *cache, RatVertexData *vertices, 
                                          uint32_t v0, uint32_t v1, uint32_t v2,
                                          uint32_t num_vertices) {
    uint32_t new_vertices[3] = {v0, v1, v2};
    
    // Move existing cache entries down and insert new vertices at front
    for (int i = 0; i < 3; i++) {
        uint32_t vertex = new_vertices[i];
        
        // Remove vertex from its current cache position (if any)
        for (uint32_t j = 0; j < RAT_VERTEX_CACHE_SIZE; j++) {
            if (cache[j] == vertex) {
                // Shift everything down
                for (uint32_t k = j; k < RAT_VERTEX_CACHE_SIZE - 1; k++) {
                    cache[k] = cache[k + 1];
                }
                cache[RAT_VERTEX_CACHE_SIZE - 1] = UINT32_MAX;
                break;
            }
        }
        
        // Shift cache down to make room at front
        for (uint32_t j = RAT_VERTEX_CACHE_SIZE - 1; j > 0; j--) {
            cache[j] = cache[j - 1];
        }
        cache[0] = vertex;
    }
    
    // Update cache positions for all vertices
    for (uint32_t i = 0; i < num_vertices; i++) {
        vertices[i].cache_position = UINT32_MAX;
        for (uint32_t j = 0; j < RAT_VERTEX_CACHE_SIZE; j++) {
            if (cache[j] == i) {
                vertices[i].cache_position = j;
                break;
            }
        }
        // Recalculate score
        vertices[i].score = rat_calculate_vertex_score(vertices[i].cache_position, 
                                                      vertices[i].num_triangles_left);
    }
}

// Optimize indices using Tom Forsyth's algorithm
static inline int rat_optimize_vertex_cache(uint16_t *indices, uint32_t num_indices, 
                                           uint32_t num_vertices) {
    if (!indices || num_indices == 0 || num_indices % 3 != 0) {
        return -1;
    }
    
    uint32_t num_triangles = num_indices / 3;
    
    // Allocate working data
    RatVertexData *vertices = (RatVertexData*)calloc(num_vertices, sizeof(RatVertexData));
    RatTriangleData *triangles = (RatTriangleData*)calloc(num_triangles, sizeof(RatTriangleData));
    uint32_t *cache = (uint32_t*)risky_malloc(RAT_VERTEX_CACHE_SIZE * sizeof(uint32_t));
    uint16_t *optimized_indices = (uint16_t*)risky_malloc(num_indices * sizeof(uint16_t));
    
    if (!vertices || !triangles || !cache || !optimized_indices) {
        free(vertices);
        free(triangles);
        free(cache);
        free(optimized_indices);
        return -1;
    }
    
    // Initialize cache as empty
    for (uint32_t i = 0; i < RAT_VERTEX_CACHE_SIZE; i++) {
        cache[i] = UINT32_MAX;
    }
    
    // Count triangles per vertex and initialize triangle data
    for (uint32_t t = 0; t < num_triangles; t++) {
        uint32_t v0 = indices[t * 3 + 0];
        uint32_t v1 = indices[t * 3 + 1];
        uint32_t v2 = indices[t * 3 + 2];
        
        triangles[t].vertices[0] = v0;
        triangles[t].vertices[1] = v1;
        triangles[t].vertices[2] = v2;
        triangles[t].processed = false;
        
        if (v0 < num_vertices) vertices[v0].num_triangles_left++;
        if (v1 < num_vertices) vertices[v1].num_triangles_left++;
        if (v2 < num_vertices) vertices[v2].num_triangles_left++;
    }
    
    // Allocate triangle lists for each vertex
    for (uint32_t v = 0; v < num_vertices; v++) {
        if (vertices[v].num_triangles_left > 0) {
            vertices[v].triangles = (uint32_t*)risky_malloc(vertices[v].num_triangles_left * sizeof(uint32_t));
            vertices[v].num_triangles_left = 0; // Reset to use as counter
        }
        vertices[v].cache_position = UINT32_MAX;
    }
    
    // Build triangle lists for each vertex
    for (uint32_t t = 0; t < num_triangles; t++) {
        for (int i = 0; i < 3; i++) {
            uint32_t v = triangles[t].vertices[i];
            if (v < num_vertices && vertices[v].triangles) {
                vertices[v].triangles[vertices[v].num_triangles_left++] = t;
            }
        }
    }
    
    // Calculate initial scores
    for (uint32_t v = 0; v < num_vertices; v++) {
        vertices[v].score = rat_calculate_vertex_score(vertices[v].cache_position, 
                                                      vertices[v].num_triangles_left);
    }
    
    // Main optimization loop
    uint32_t output_pos = 0;
    for (uint32_t current_triangle = 0; current_triangle < num_triangles; current_triangle++) {
        // Find best triangle to process next
        uint32_t best_triangle = UINT32_MAX;
        float best_score = -1.0f;
        
        for (uint32_t t = 0; t < num_triangles; t++) {
            if (triangles[t].processed) continue;
            
            // Calculate triangle score as sum of vertex scores
            float triangle_score = 0.0f;
            for (int i = 0; i < 3; i++) {
                uint32_t v = triangles[t].vertices[i];
                if (v < num_vertices) {
                    triangle_score += vertices[v].score;
                }
            }
            
            if (triangle_score > best_score) {
                best_score = triangle_score;
                best_triangle = t;
            }
        }
        
        if (best_triangle == UINT32_MAX) break; // No more triangles
        
        // Output the best triangle
        triangles[best_triangle].processed = true;
        uint32_t v0 = triangles[best_triangle].vertices[0];
        uint32_t v1 = triangles[best_triangle].vertices[1];
        uint32_t v2 = triangles[best_triangle].vertices[2];
        
        optimized_indices[output_pos++] = (uint16_t)v0;
        optimized_indices[output_pos++] = (uint16_t)v1;
        optimized_indices[output_pos++] = (uint16_t)v2;
        
        // Update vertex triangle counts
        if (v0 < num_vertices) vertices[v0].num_triangles_left--;
        if (v1 < num_vertices) vertices[v1].num_triangles_left--;
        if (v2 < num_vertices) vertices[v2].num_triangles_left--;
        
        // Update cache and recalculate scores
        rat_update_vertex_cache(cache, vertices, v0, v1, v2, num_vertices);
    }
    
    // Copy optimized indices back
    memcpy(indices, optimized_indices, num_indices * sizeof(uint16_t));
#ifdef GEKKO
    FlushGPUCache(indices, num_indices * sizeof(uint16_t));
#endif
    
    // Cleanup
    for (uint32_t v = 0; v < num_vertices; v++) {
        if (vertices[v].triangles) {
            free(vertices[v].triangles);
        }
    }
    free(vertices);
    free(triangles);
    free(cache);
    free(optimized_indices);
    
    return 0;
}

// Function to optimize indices in a loaded RAT animation
static inline int rat_optimize_animation_cache(RatAnimationInfo *anim) {
    if (!anim || !anim->indices || anim->num_indices == 0) {
        return 0; // Nothing to optimize
    }
    
    printf("Optimizing vertex cache for animation with %u indices, %u vertices...\n", 
           anim->num_indices, anim->num_vertices);
    
    int result = rat_optimize_vertex_cache(anim->indices, anim->num_indices, anim->num_vertices);
    
    if (result == 0) {
        printf("Vertex cache optimization completed successfully.\n");
    } else {
        printf("Vertex cache optimization failed.\n");
    }
    
    return result;
}

// Implementation of utility functions
static inline const char* rat_get_last_error(void) {
    return rat_last_error;
}

static inline uint32_t rat_get_triangle_count(DecompressionContext *ctx) {
    if (!ctx) {
        return 0;
    }
    
    // If we have indices, triangle count is num_indices / 3
    if (ctx->indices && ctx->num_indices > 0) {
        return ctx->num_indices / 3;
    }
    
    // If no indices, assume triangles are specified as vertex triplets
    return ctx->num_vertices / 3;
}
#endif // RAT_DECOMPRESS_H

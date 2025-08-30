#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>
#include <string.h>
#include <stdio.h>

typedef struct { float x, y, z; } Vec3;
typedef struct { uint8_t u, v; } Vec2;        // Changed to uint8_t for UVs
typedef struct { uint8_t r, g, b; } Color3;   // Changed to uint8_t for colors

// Manual byte swap functions for TCC compatibility
static inline uint32_t manual_bswap32(uint32_t x) {
    return ((x & 0xff000000) >> 24) |
           ((x & 0x00ff0000) >> 8) |
           ((x & 0x0000ff00) << 8) |
           ((x & 0x000000ff) << 24);
}

static inline uint16_t manual_bswap16(uint16_t x) {
    return ((x & 0xff00) >> 8) |
           ((x & 0x00ff) << 8);
}
/**
 * @brief Convert file path for N64 compatibility
 * 
 * On N64, replaces "sourcefiles/" and "assets/" with "rom:/"
 * Also converts file extensions: .png -> .sprite, .wav -> .audio
 * On other platforms, returns the original path unchanged
 * 
 * @param path The original file path
 * @param buffer_index Which static buffer to use (0-3) to avoid conflicts
 * @return const char* The converted path (static buffer, not thread-safe)
 */
static inline const char* convert_path_for_n64_impl(const char* path, int buffer_index) {
    if (!path) return path;
    
#ifdef N64
    static char converted_paths[4][256];  // Multiple buffers to avoid conflicts
    char* converted_path = converted_paths[buffer_index % 4];
    const char* file_part = path;
    
    // Check if path starts with "sourcefiles/" or "assets/"
    if (strncmp(path, "sourcefiles/", 12) == 0) {
        file_part = path + 12;
        snprintf(converted_path, 256, "rom:/%s", file_part);
    } else if (strncmp(path, "assets/", 7) == 0) {
        file_part = path + 7;
        snprintf(converted_path, 256, "rom:/%s", file_part);
    } else {
        // If path doesn't start with these prefixes, copy as-is for extension conversion
        strncpy(converted_path, path, 255);
        converted_path[255] = '\0';
    }
    
    // Convert file extensions for N64 - only for specific file types
    char* ext_pos = strrchr(converted_path, '.');
    if (ext_pos) {
        if (strcmp(ext_pos, ".png") == 0) {
            strcpy(ext_pos, ".sprite");
        } else if (strcmp(ext_pos, ".wav") == 0) {
            strcpy(ext_pos, ".audio");
        }
        // Note: .rat, .emu, .cam, and other files keep their original extensions
    }
    
    return converted_path;
#else
    // On non-N64 platforms, return path unchanged
    return path;
#endif
}

// Main conversion function - uses buffer 0
static inline const char* convert_path_for_n64(const char* path) {
    return convert_path_for_n64_impl(path, 0);
}

// Additional conversion functions for when multiple conversions are needed in the same call
static inline const char* convert_path_for_n64_alt(const char* path) {
    return convert_path_for_n64_impl(path, 1);
}

/**
 * @brief Test function to verify path conversion works correctly
 * Call this during initialization to verify path handling
 */
static inline void test_path_conversion() {
#ifdef N64
    printf("N64 Path Conversion Test:\n");
    printf("  'sourcefiles/test.rat' -> '%s'\n", convert_path_for_n64("sourcefiles/test.rat"));
    printf("  'assets/texture.png' -> '%s'\n", convert_path_for_n64("assets/texture.png"));
    printf("  'sourcefiles/music.wav' -> '%s'\n", convert_path_for_n64("sourcefiles/music.wav"));
    printf("  'other/file.txt' -> '%s'\n", convert_path_for_n64("other/file.txt"));
    printf("  'test.png' -> '%s'\n", convert_path_for_n64("test.png"));
    printf("Buffer conflict test:\n");
    const char* rat_path = convert_path_for_n64("sourcefiles/yaris.rat");
    const char* tex_path = convert_path_for_n64_alt("assets/yaris.png");
    printf("  RAT: '%s', TEX: '%s'\n", rat_path, tex_path);
#else
    printf("Non-N64 Platform: Path conversion disabled\n");
#endif
}

#endif
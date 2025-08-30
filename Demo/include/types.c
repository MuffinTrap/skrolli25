/*
 * types.c - Implementation for type-related utility functions
 */

#include "types.h"
#include <string.h>
#include <stdio.h>

// Implementation of the path conversion helper, now local to this file
// On N64, this function is essential for mapping PC paths to the ROM filesystem.
static const char* convert_path_for_n64_impl(const char* path, int buffer_index) {
    if (!path) return path;
    
#ifdef N64
    // Use multiple static buffers to allow for a few simultaneous path conversions
    // without overwriting the previous result. This is useful for functions that
    // need to convert multiple paths in a single expression.
    static char converted_paths[4][256];
    char* converted_path = converted_paths[buffer_index % 4];
    const char* file_part = path;
    
    // If the path starts with known source directories, replace them with "rom:/"
    if (strncmp(path, "sourcefiles/", 12) == 0) {
        file_part = path + 12;
        snprintf(converted_path, 256, "rom:/%s", file_part);
    } else if (strncmp(path, "assets/", 7) == 0) {
        file_part = path + 7;
        snprintf(converted_path, 256, "rom:/%s", file_part);
    } else {
        // Otherwise, copy the path as-is for extension conversion
        strncpy(converted_path, path, 255);
        converted_path[255] = '\0';
    }
    
    // Convert file extensions for N64-specific formats
    char* ext_pos = strrchr(converted_path, '.');
    if (ext_pos) {
        if (strcmp(ext_pos, ".png") == 0) {
            strcpy(ext_pos, ".sprite");
        } else if (strcmp(ext_pos, ".wav") == 0) {
            strcpy(ext_pos, ".audio");
        }
        // Other extensions (.rat, .emu, etc.) are unchanged
    }
    
    return converted_path;
#else
    // On non-N64 platforms, the path is returned unchanged.
    return path;
#endif
}

// Main conversion function, uses the first buffer.
const char* convert_path_for_n64(const char* path) {
    return convert_path_for_n64_impl(path, 0);
}

// Alternative conversion function, uses a different buffer to avoid conflicts.
const char* convert_path_for_n64_alt(const char* path) {
    return convert_path_for_n64_impl(path, 1);
}

// Test function to verify that path conversion is working as expected.
void test_path_conversion(void) {
#ifdef N64
    printf("N64 Path Conversion Test:\n");
    printf("  'sourcefiles/test.rat' -> '%s'\n", convert_path_for_n64("sourcefiles/test.rat"));
    printf("  'assets/texture.png' -> '%s'\n", convert_path_for_n64("assets/texture.png"));
    printf("  'sourcefiles/music.wav' -> '%s'\n", convert_path_for_n64("sourcefiles/music.wav"));
    printf("  'other/file.txt' -> '%s'\n", convert_path_for_n64("other/file.txt"));
    printf("  'test.png' -> '%s'\n", convert_path_for_n64("test.png"));
    
    // Verify that using the main and alt functions simultaneously works correctly
    printf("Buffer conflict test:\n");
    const char* rat_path = convert_path_for_n64("sourcefiles/yaris.rat");
    const char* tex_path = convert_path_for_n64_alt("assets/yaris.png");
    printf("  RAT: '%s', TEX: '%s'\n", rat_path, tex_path);
#else
    printf("Non-N64 Platform: Path conversion disabled.\n");
#endif
}

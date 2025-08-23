/**
 * @file texture_manager.h
 * @brief Texture management system for Ziz/N64/Gecko/OpenGL applications
 */

#pragma once
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifdef N64
    #include <libdragon.h>
    #include <GL/gl.h>
    #include <GL/glu.h>
    #include <GL/gl_integration.h>
#else
    #include "GL_macros.h"  
    #define STBI_NO_SIMD
    #define STB_IMAGE_IMPLEMENTATION
    #include <stb_image.h>
    #include <surface.h>
    typedef struct {
        unsigned char *data;
        int width;
        int height;
        int channels;
    } sprite_t;
#endif


/** Maximum number of textures that can be managed simultaneously */
#define MAX_TEXTURES 32

/** Identifier value representing an invalid texture */
#define INVALID_TEXTURE_ID -1


/**
 * @struct TextureEntry
 * @brief Metadata container for managed textures
 * 
 * @var filename  Original texture filename (max 255 chars + null)
 * @var hash      Precomputed filename hash for quick comparisons
 * @var last_used Frame counter timestamp for LRU management
 */
typedef struct {
    char filename[256];
    unsigned long hash;
    size_t last_used;
} TextureEntry;

/** @brief Array of loaded sprite pointers */
static sprite_t *sprites[MAX_TEXTURES];

/** @brief OpenGL texture IDs corresponding to loaded sprites */
static GLuint spriteVRAM_id[MAX_TEXTURES];

/** @brief Texture metadata entries */
static TextureEntry texture_entries[MAX_TEXTURES];

/** @brief Current number of managed textures */
static size_t texture_pool_size = 0;

/** @brief Global frame counter for LRU tracking */
static size_t lru_counter = 0;

/** @brief Currently bound OpenGL texture ID to minimize state changes */
static GLuint current_bound_texture = 0;

/* ====================== */
/* Utility Functions      */
/* ====================== */

#ifndef N64

sprite_t *sprite_load(const char *filename) {
    sprite_t *sprite = malloc(sizeof(sprite_t));
    if (!sprite) return NULL;

    sprite->data = stbi_load(filename, &sprite->width, &sprite->height, &sprite->channels, 0);
    if (!sprite->data) {
        printf("stbi_load failed for file: %s\n", filename);
        free(sprite);
        return NULL;
    }

    return sprite;
}

void sprite_free(sprite_t *sprite) {
    if (!sprite) return;
    if (sprite->data) stbi_image_free(sprite->data);
    free(sprite);
}

#endif

/**
 * @brief Compute 32-bit hash for filename strings
 * @param str Null-terminated filename string
 * @return djb2 hash value for quick comparisons
 */
unsigned long compute_hash(const char *str) {
    unsigned long hash = 5381;
    int c;
    while ((c = (unsigned char)*str++))
        hash = ((hash << 5) + hash) + c; // hash * 33 + c
    return hash;
}

/**
 * @brief Find texture ID by filename using hash comparison
 * @param filename Texture filename to search for
 * @return Texture ID or INVALID_TEXTURE_ID if not found
 */
int findTextureByFilename(const char* filename) {
    if (!filename) return INVALID_TEXTURE_ID;
    unsigned long target_hash = compute_hash(filename);
    for (size_t i = 0; i < texture_pool_size; i++) {
        if (texture_entries[i].hash == target_hash &&
            strcmp(texture_entries[i].filename, filename) == 0) {
            return i;
        }
    }
    return INVALID_TEXTURE_ID;
}

/* ====================== */
/* Core Functions         */
/* ====================== */

/**
 * @brief Load a texture into management system
 * @param filename Path to texture file
 * @return Texture ID or INVALID_TEXTURE_ID on failure
 * 
 * @note Reuses existing texture if already loaded
 * @warning Blocks if texture pool is full
 */
int addTexture(const char *filename) {
    int existing_id = findTextureByFilename(filename);
    if (existing_id != INVALID_TEXTURE_ID) {
        return existing_id;
    }

    if (texture_pool_size >= MAX_TEXTURES) {
        return INVALID_TEXTURE_ID;
    }

    int id = texture_pool_size;
    sprites[id] = sprite_load(filename);
    if (!sprites[id]) {
        return INVALID_TEXTURE_ID;
    }

    // Store filename and hash
    strncpy(texture_entries[id].filename, filename, sizeof(texture_entries[id].filename) - 1);
    texture_entries[id].filename[sizeof(texture_entries[id].filename) - 1] = '\0';
    texture_entries[id].hash = compute_hash(filename);
    texture_entries[id].last_used = 0;

    texture_pool_size++;
    return id;
}

/**
 * @brief Release OpenGL resources for a texture
 * @param id Texture ID to unload
 */
void unloadTextureFromGL(int id) {
    if (spriteVRAM_id[id] != 0) {
        glDeleteTextures(1, &spriteVRAM_id[id]);
        spriteVRAM_id[id] = 0;
        texture_entries[id].last_used = 0;
    }
}

/**
 * @brief Compare function for qsort LRU implementation
 * @param a First texture ID pointer
 * @param b Second texture ID pointer
 * @return Difference in last_used timestamps
 */
static int compare_last_used(const void *a, const void *b) {
    int id_a = *(const int*)a;
    int id_b = *(const int*)b;
    return (texture_entries[id_a].last_used - texture_entries[id_b].last_used);
}
void set_nearest() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
}
void set_linear() {
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
}
/**
 * @brief Bind texture to OpenGL context
 * @param id Texture ID to bind
 * @return Bound OpenGL texture ID or 0 on failure
 * 
 * @note Implements LRU unloading when TMEM is full
 * @warning Invalidates other textures' VRAM IDs if unloading occurs
 */
int bind_texture(int id) {
    if (id < 0 || id >= texture_pool_size) return 0;

    if (spriteVRAM_id[id] != 0) {
        if (current_bound_texture != spriteVRAM_id[id]) {
            glBindTexture(GL_TEXTURE_2D, spriteVRAM_id[id]);
            current_bound_texture = spriteVRAM_id[id];
        }
        texture_entries[id].last_used = ++lru_counter;
        return spriteVRAM_id[id];
    }

    #ifdef N64
    // LRU-based unloading
    if (!sprite_fits_tmem(sprites[id])) {
        int loaded_ids[MAX_TEXTURES], count = 0;
        for (int i = 0; i < texture_pool_size; ++i) {
            if (spriteVRAM_id[i] != 0) loaded_ids[count++] = i;
        }
        qsort(loaded_ids, count, sizeof(int), compare_last_used);
        for (int i = 0; i < count; ++i) {
            unloadTextureFromGL(loaded_ids[i]);
            if (sprite_fits_tmem(sprites[id])) break;
        }
    }
    #endif

    GLuint texID;
    glGenTextures(1, &texID);
    glBindTexture(GL_TEXTURE_2D, texID);
    current_bound_texture = texID;

    #ifdef N64
        glSpriteTextureN64(
            GL_TEXTURE_2D, 
            sprites[id],
            &(rdpq_texparms_t){
                .s.repeats = REPEAT_INFINITE,
                .t.repeats = REPEAT_INFINITE,
                .s.mirror = 0,
                .t.mirror = 0
            });
    #else
        GLenum internalFormat, format;
        switch(sprites[id]->channels) {
            case 1: 
                internalFormat = GL_LUMINANCE;
                format = GL_RED;
                break;
            case 3: 
                internalFormat = GL_RGB8;
                format = GL_RGB;
                break;
            case 4: 
                internalFormat = GL_RGBA8;
                format = GL_RGBA;
                break;
            default:
                printf("Unsupported channel count: %d\n", sprites[id]->channels);
                return INVALID_TEXTURE_ID;
        }
        glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, sprites[id]->width, sprites[id]->height,
                0, format, GL_UNSIGNED_BYTE, sprites[id]->data);
    #endif
    // Set default filtering that will be overridden by the caller
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    spriteVRAM_id[id] = texID;
    texture_entries[id].last_used = ++lru_counter;
    return texID;
}

/**
 * @brief Release all texture resources
 * 
 * @note Must be called before application exit to prevent leaks
 * @warning Invalidates all texture IDs
 */
void freeTexturePool() {
    for (size_t i = 0; i < texture_pool_size; ++i) {
        unloadTextureFromGL(i);
        sprite_free(sprites[i]);
    }
    texture_pool_size = 0;
    lru_counter = 0;
    current_bound_texture = 0;
}

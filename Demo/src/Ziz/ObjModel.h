#ifndef OBJMODEL_H
#define OBJMODEL_H

// #include <cgltf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GL_SILENCE_DEPRECATION 1
#include <opengl_include.h>

// ERROR CODES
#define OBJ_MODEL_LIMIT_REACHED -2
#define OBJ_SHORT_DATA -3
#define OBJ_UNKNOWN_FORMAT -4
#define OBJ_INVALID_JSON -5
#define OBJ_INVALID_GLTF -6
#define OBJ_FILE_NOT_FOUND -7
#define OBJ_IO_ERROR -8
#define OBJ_OUT_OF_MEMORY -9
#define OBJ_LEGACY_GLTF -10
#define OBJ_INVALID_OPTIONS -11
#define OBJ_LOAD_BUFFER_FAIL -12
#define OBJ_GLTF_NOT_VALID -13

//#include "../cglm/cglm.h"
#include "mesh.h"

int load_gltf(const char* path, const char* name);
struct Mesh load_to_mesh(int model_index);

void draw_gltf(int model_index);
#endif

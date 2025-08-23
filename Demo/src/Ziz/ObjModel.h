#ifndef OBJMODEL_H
#define OBJMODEL_H

// #include <cgltf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#define GL_SILENCE_DEPRECATION 1
#include <opengl_include.h>

//#include "../cglm/cglm.h"
#include "mesh.h"

int load_gltf(const char* path, const char* name);
struct Mesh load_to_mesh(int model_index);

void draw_gltf(int model_index);
#endif

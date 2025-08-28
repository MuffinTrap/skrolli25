#ifndef TEXTURE_H
#define TEXTURE_H

#include "GL_macros.h"

int addTexture(const char* filename);

int bind_texture(int id);

int get_texture_width(int id);
int get_texture_height(int id);

#endif // TEXTURE_H

#ifndef GRADIENT_MESH_H
#define GRADIENT_MESH_H
// Uses gradient and shape
// has texture
// animated vertices and colors
#include "gradient.h"
#include <m_math.h>

#include <opengl_include.h>

enum GradientShape
{
    GradientHorizontal,
    GradientVertical,
    GradientCircle
};

struct GradientMesh
{
    struct Gradient* gradient;

    // Enough space for all shapes
    float3 vertices[GRADIENT_SIZE * 3];
    float2 uvs[GRADIENT_SIZE * 3];

    short vertices_used;
    GLuint gl_texture_name;

    enum GradientShape shape;
};

struct GradientMesh GradientMesh_Create(struct Gradient* gradient, GLuint gl_texture_name);
void GradientMesh_Draw(struct GradientMesh* mesh);



#endif

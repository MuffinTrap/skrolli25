#ifndef BUNNY_FX_H
#define BUNNY_FX_H

#include "../Ziz/mesh.h"

enum MeshDrawMode
{
    DrawLines,
    DrawTriangles,
    DrawMatcap
};

struct Bunny
{
    int obj_id;
    struct Mesh mesh;
};

struct Bunny Bunny_Load(const char* filename);

void Bunny_Draw_immediate(struct Bunny* bunny, enum MeshDrawMode draw_mode);

void Bunny_Draw_mesh(struct Bunny* bunny, enum MeshDrawMode draw_mode);

#endif

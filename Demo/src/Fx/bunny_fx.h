#ifndef BUNNY_FX_H
#define BUNNY_FX_H

#include "../Ziz/mesh.h"

#   ifdef GEKKO
#include "ufbx_to_mesh.h"
#   endif

enum BunnyFormat
{
    Bunny_GLTF,
    Bunny_FBX
};

struct Bunny
{
    enum BunnyFormat format;
    // CGLTF
    int obj_id;
    int error_code;

    // UFBX
    void* fbx_mesh;

    struct Mesh mesh;
};

struct Bunny Bunny_Load_GLTF(const char* filename);
struct Bunny Bunny_Load_UFBX(const char* filename);

void Bunny_Allocate_Texcoords(struct Bunny* bunny);
void Bunny_Draw_immediate(struct Bunny* bunny);
void Bunny_Draw_mesh(struct Bunny* bunny, enum MeshDrawMode draw_mode);
void Bunny_Draw_mesh_partial(struct Bunny* bunny, enum MeshDrawMode draw_mode, int percentage);

#endif

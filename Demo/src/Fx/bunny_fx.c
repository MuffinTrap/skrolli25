#include "bunny_fx.h"
#include "../Ziz/ObjModel.h"

struct Bunny Bunny_Load(const char* filename)
{
    struct Bunny bnuy;
    bnuy.obj_id = load_gltf(filename, "bunny_medium");
    printf("Loaded to model id %d\n", bnuy.obj_id);
    if (bnuy.obj_id >= 0)
    {
        bnuy.mesh = load_to_mesh(bnuy.obj_id);
    }
    else
    {
        printf("Could not load gltf %s\n", filename);
    }
    return bnuy;
}

void Bunny_Draw_immediate(struct Bunny* bunny, enum MeshDrawMode draw_mode)
{
    draw_gltf(bunny->obj_id);
}

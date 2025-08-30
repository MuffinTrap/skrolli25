#include "bunny_fx.h"
#include "../Ziz/ObjModel.h"
#include <types.h>
#include <animation.h>

struct Bunny Bunny_Load_GLTF(const char* filename)
{
    struct Bunny bunny;
    bunny.obj_id = load_gltf(filename, "bunny_medium");
    printf("Loaded to model id %d\n", bunny.obj_id);
    if (bunny.obj_id >= 0)
    {
        printf("Loading bunny to mesh\n");
        bunny.mesh = load_to_mesh(bunny.obj_id);
        Mesh_PrintInfo(&bunny.mesh);
        bunny.format = Bunny_GLTF;
    }
    else
    {
        bunny.error_code = bunny.obj_id;
        printf("Could not load gltf %s\n", filename);
    }
    return bunny;
}

struct Bunny Bunny_Load_RAT(const char* filename)
{
    struct Bunny bunny;
    bunny.rat = rat_model_create("bunny_medium", "assets/bunny_medium.rat", NULL);

    bunny.format = Bunny_RAT;
    return bunny;
}

void Bunny_Draw_immediate(struct Bunny* bunny)
{
    draw_gltf(bunny->obj_id);
}

void Bunny_Draw_mesh(struct Bunny* bunny, enum MeshDrawMode draw_mode)
{
    switch(bunny->format)
    {
        case Bunny_GLTF:
    Mesh_DrawElements(&bunny->mesh, draw_mode);
    break;
        case Bunny_RAT:
            rat_model_render(bunny->rat);
            break;
    }
}

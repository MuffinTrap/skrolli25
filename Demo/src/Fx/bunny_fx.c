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
        Mesh_PrintInfo(&bunny.mesh, false);
        bunny.format = Bunny_GLTF;
    }
    else
    {
        bunny.error_code = bunny.obj_id;
        printf("Could not load gltf %s\n", filename);
    }
    return bunny;
}

struct Bunny Bunny_Load_UFBX(const char* filename)
{
    struct Bunny bunny;
#   ifdef GEKKO
    bunny.fbx_mesh = Ufbx_GetFirstMesh(filename);
    bunny.mesh = Ufbx_LoadToMesh(bunny.fbx_mesh);
#endif

    bunny.format = Bunny_FBX;
    return bunny;

}

void Bunny_Allocate_Texcoords(struct Bunny* bunny)
{
    Mesh_Allocate(&bunny->mesh, bunny->mesh.vertex_count, (AttributeTexcoord));
}

void Bunny_Draw_immediate(struct Bunny* bunny)
{
    draw_gltf(bunny->obj_id);
}

void Bunny_Draw_mesh_partial(struct Bunny* bunny, enum MeshDrawMode draw_mode, int percentage)
{
    switch(bunny->format)
    {
        case Bunny_GLTF:
            Mesh_DrawPartial(&bunny->mesh, draw_mode, percentage);
        break;
        case Bunny_FBX:
#           ifdef GEKKO
            Mesh_DrawPartial(&bunny->mesh, draw_mode, percentage);
           // Ufbx_DrawMesh(bunny->ufbx_mesh);
#           endif

            break;
    }

}

void Bunny_Draw_mesh(struct Bunny* bunny, enum MeshDrawMode draw_mode)
{
    switch(bunny->format)
    {
        case Bunny_GLTF:
            Mesh_Draw(&bunny->mesh, draw_mode);
        break;
        case Bunny_FBX:
#           ifdef GEKKO
            Mesh_Draw(&bunny->mesh, draw_mode);
           // Ufbx_DrawMesh(bunny->ufbx_mesh);
#           endif

            break;
    }
}

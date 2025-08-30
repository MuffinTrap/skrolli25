#include "ufbx_to_mesh.h"
#include <stdio.h>
#include "../ufbx/ufbx.h"


struct Mesh* Ufbx_LoadMesh(const char* fbx_filename)
{
    ufbx_scene *scene = ufbx_load_file(fbx_filename, NULL, NULL);

    for (size_t i = 0; i < scene->nodes.count; i++) {
        ufbx_node *node = scene->nodes.data[i];
        printf("%s\n", node->name.data);
    }

}

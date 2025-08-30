#include "ufbx_to_mesh.h"
#include <stdio.h>
#include "../Ziz/screenprint.h"
static ufbx_scene *scene = NULL;

void Ufbx_DrawMesh(ufbx_mesh* mesh)
{
    glBegin(GL_TRIANGLES);
    for (size_t i = 0; i < mesh->faces.count; i++) {
        ufbx_face face = mesh->faces.data[i];

        // Loop through the corners of the polygon.
        for (uint32_t corner = 0; corner < face.num_indices; corner++) {

            // Faces are defined by consecutive indices, one for each corner.
            uint32_t index = face.index_begin + corner;

            // Retrieve the position, normal and uv for the vertex.
            ufbx_vec3 position = ufbx_get_vertex_vec3(&mesh->vertex_position, index);
            ufbx_vec3 normal = ufbx_get_vertex_vec3(&mesh->vertex_normal, index);
            //ufbx_vec2 uv = ufbx_get_vertex_vec2(&mesh->vertex_uv, index);

            glNormal3f(normal.x, normal.y, normal.z);
            glVertex3f(position.x, position.y, position.z);
            //polygon_corner(position, normal, uv);
        }

    }
    glEnd();
}

struct Mesh Ufbx_LoadToMesh(ufbx_mesh* fbx_mesh)
{
	struct Mesh mesh = Mesh_CreateEmpty();
	size_t vertices = fbx_mesh->num_triangles * 3;
    Mesh_Allocate(&mesh, vertices, (AttributePosition|AttributeNormal|AttributeTexcoord));

    int vertex_index = 0;

    for (size_t i = 0; i < fbx_mesh->faces.count; i++) {
        ufbx_face face = fbx_mesh->faces.data[i];

        // Loop through the corners of the polygon.
        for (uint32_t corner = 0; corner < face.num_indices; corner++) {

            // Faces are defined by consecutive indices, one for each corner.
            uint32_t index = face.index_begin + corner;

            // Retrieve the position, normal and uv for the vertex.
            ufbx_vec3 position = ufbx_get_vertex_vec3(&fbx_mesh->vertex_position, index);
            ufbx_vec3 normal = ufbx_get_vertex_vec3(&fbx_mesh->vertex_normal, index);
            //ufbx_vec2 uv = ufbx_get_vertex_vec2(&fbx_mesh->vertex_uv, index);

            int v = vertex_index * 3;
            mesh.positions[v+0] = position.x;
            mesh.positions[v+1] = position.y;
            mesh.positions[v+2] = position.z;
            mesh.normals[v+0] = normal.x;
            mesh.normals[v+1] = normal.y;
            mesh.normals[v+2] = normal.z;
            vertex_index++;
        }
    }
    return mesh;
}

ufbx_mesh* Ufbx_GetFirstMesh(const char* fbx_filename)
{
    if (scene == NULL)
    {
        scene = ufbx_load_file(fbx_filename, NULL, NULL);
    }

    if (scene != NULL)
    {
        screenprintf("Loaded %s", fbx_filename);
        for (size_t i = 0; i < scene->nodes.count; i++) {
            ufbx_node *node = scene->nodes.data[i];
            screenprintf("Node %d : %s", i, node->name.data);
            if (node->mesh != nullptr)
            {
                ufbx_mesh* mesh = node->mesh;

                screenprintf("Mesh %s (%u) with %zu faces", mesh->name.data, mesh->element_id, mesh->faces.count);
                if (mesh->vertex_normal.exists)
                {
                    screenprintf(", %zu normals", mesh->vertex_normal.values.count);
                }
                if (mesh->vertex_uv.exists)
                {
                    screenprintf(",%zu uvs", mesh->vertex_uv.values.count);
                }
                return mesh;
            }
        }
    }
    return NULL;
}

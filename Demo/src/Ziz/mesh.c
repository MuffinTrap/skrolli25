#include "mesh.h"

struct Mesh Mesh_CreateEmpty(void)
{
    struct Mesh mesh;
    mesh.positions = NULL;
    mesh.normals = NULL;
    mesh.texcoords = NULL;
    mesh.vertex_count = 0;
    mesh.indices = NULL;
    mesh.index_count = 0;
    return mesh;
}

void Mesh_Allocate(struct Mesh* mesh, int vertex_count, int attribute_bitfield)
{

    if ((attribute_bitfield & AttributePosition) != 0)
    {
        int bytes = sizeof(float) * vertex_count * 3;
        if (mesh->positions == NULL)
        {
            printf("Allocation of mesh for %d vertices\n", vertex_count);
            mesh->positions = (float*)malloc(bytes);
        }
    }

    if ((attribute_bitfield & AttributeNormal) != 0)
    {
        if (mesh->normals == NULL)
        {
            mesh->normals = (float*)malloc(sizeof(float) * vertex_count * 3);
        }
    }

    if ((attribute_bitfield & AttributeTexcoord) != 0)
    {
        if (mesh->texcoords == NULL)
        {
            mesh->texcoords = (float*)malloc(sizeof(float) * vertex_count * 2);
        }
    }

    mesh->allocated_vertex_count = M_MAX(vertex_count, mesh->allocated_vertex_count);
    mesh->vertex_count = vertex_count;
    mesh->indices = NULL;
    mesh->index_count = 0;
}

void Mesh_DrawArrays(struct Mesh* mesh)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, mesh->positions);
	//glDrawElements(GL_TRIANGLES, mesh->indexCount, GL_UNSIGNED_SHORT, mesh->indices);
    glDrawArrays(GL_TRIANGLES, 0, mesh->vertex_count);
    glDisableClientState(GL_VERTEX_ARRAY);
}

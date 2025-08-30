#include "mesh.h"
#include <opengl_include.h>
#include <wii_memory_functions.h>
#include "screenprint.h"

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
            mesh->positions = (float*)AllocateGPUMemory(bytes);
        }
    }

    if ((attribute_bitfield & AttributeNormal) != 0)
    {
        if (mesh->normals == NULL)
        {
            mesh->normals = (float*)AllocateGPUMemory(sizeof(float) * vertex_count * 3);
        }
    }

    if ((attribute_bitfield & AttributeTexcoord) != 0)
    {
        if (mesh->texcoords == NULL)
        {
            mesh->texcoords = (float*)AllocateGPUMemory(sizeof(float) * vertex_count * 2);
        }
    }

    mesh->allocated_vertex_count = M_MAX(vertex_count, mesh->allocated_vertex_count);
    mesh->vertex_count = vertex_count;
    mesh->indices = NULL;
    mesh->index_count = 0;
}

static void Setup_Arrays(struct Mesh* mesh)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, mesh->positions);
    if(mesh->normals != NULL)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, mesh->normals);
        screenprintf("Mesh uses normals");
    }

}
static void Disable_Arrays(struct Mesh* mesh)
{
    if(mesh->normals != NULL)
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
}

void Mesh_DrawElements(struct Mesh* mesh, enum MeshDrawMode mode)
{
    Setup_Arrays(mesh);
    GLenum gl_mode = GL_TRIANGLES;
    if (mode == DrawLines)
    {
        glPolygonMode(GL_FRONT, GL_LINE);
    }
    else if (mode == DrawPoints)
    {
        gl_mode = GL_POINTS;
    }
	glDrawElements(gl_mode, mesh->index_count, GL_UNSIGNED_SHORT, mesh->indices);
    glPolygonMode(GL_FRONT, GL_FILL);
    Disable_Arrays(mesh);

}

void Mesh_DrawArrays(struct Mesh* mesh, enum MeshDrawMode mode)
{
    Setup_Arrays(mesh);
    GLenum gl_mode = GL_TRIANGLES;
    if (mode == DrawLines)
    {
        glPolygonMode(GL_FRONT, GL_LINE);
    }
    else if (mode == DrawPoints)
    {
        gl_mode = GL_POINTS;
    }
    glDrawArrays(gl_mode, 0, mesh->vertex_count);
    glPolygonMode(GL_FRONT, GL_FILL);
    Disable_Arrays(mesh);
}

void Mesh_PrintInfo(struct Mesh* mesh)
{
    if (mesh->positions != NULL)
    {
        screenprintf("Mesh has %d positions\n", mesh->vertex_count);
    }
    if (mesh->normals != NULL)
    {
        screenprintf("Mesh has %d normals\n", mesh->vertex_count);
    }
    if (mesh->texcoords != NULL)
    {
        screenprintf("Mesh has %d texcoords\n", mesh->vertex_count);
    }
    if (mesh->indices != NULL)
    {
        screenprintf("Mesh has %d indices\n", mesh->index_count);
    }
}

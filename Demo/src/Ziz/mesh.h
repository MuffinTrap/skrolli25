#ifndef MESH_H
#define MESH_H

enum MeshDrawMode
{
    DrawPoints,
    DrawLines,
    DrawTriangles,
    DrawMatcap
};

struct Mesh
{
    float* positions;
    float* normals;
    float* texcoords;
    unsigned short* indices;
    int vertex_count;
    int index_count;
    int allocated_vertex_count;
};

enum VertexAttribute
{
    AttributePosition = 1,
    AttributeNormal = 2,
    AttributeTexcoord = 4
};

struct Mesh Mesh_CreateEmpty(void);

void Mesh_PrintInfo(struct Mesh* mesh);
void Mesh_Allocate(struct Mesh* mesh, int vertex_count, int attribute_bitfield);

void Mesh_DrawElements(struct Mesh* mesh, enum MeshDrawMode mode);
void Mesh_DrawArrays(struct Mesh* mesh, enum MeshDrawMode mode);


#endif

#ifndef MESH_H
#define MESH_H

enum MeshDrawMode
{
    DrawPoints,
    DrawLines,
    DrawTriangles,
    DrawMatcap
};

enum VertexAttribute
{
    AttributePosition = 1,
    AttributeNormal = 2,
    AttributeTexcoord = 4
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

    unsigned int enabled_attributes;
};


struct Mesh Mesh_CreateEmpty(void);

void Mesh_PrintInfo(struct Mesh* mesh, bool to_screen);
void Mesh_Allocate(struct Mesh* mesh, int vertex_count, int attribute_bitfield);

void Mesh_DisableAttribute(struct Mesh* mesh, enum VertexAttribute attrib);
void Mesh_EnableAttribute(struct Mesh* mesh, enum VertexAttribute attrib);

void Mesh_GenerateMatcapUVs(struct Mesh* mesh);
void Mesh_Draw(struct Mesh* mesh, enum MeshDrawMode mode);
void Mesh_DrawPartial(struct Mesh* mesh, enum MeshDrawMode mode, int percentage);


#endif

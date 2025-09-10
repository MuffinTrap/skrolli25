#ifndef UFBX_TO_MESH_H
#define UFBX_TO_MESH_H

#include "../Ziz/mesh.h"
#include "../ufbx/ufbx.h"

ufbx_mesh* Ufbx_GetFirstMesh(const char* fbx_filename);
void Ufbx_FreeScene();
struct Mesh Ufbx_LoadToMesh(ufbx_mesh* mesh);
void Ufbx_DrawMesh(ufbx_mesh* mesh);


#endif

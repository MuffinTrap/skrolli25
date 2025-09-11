#include "mesh.h"
#include <opengl_include.h>
#include <wii_memory_functions.h>
#include <m_math.h>
#include <m_float2_math.h>
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
    mesh.enabled_attributes = 0;
    return mesh;
}

void Mesh_Allocate(struct Mesh* mesh, int vertex_count, int attribute_bitfield)
{

    if ((attribute_bitfield & AttributePosition) != 0)
    {
        int bytes = sizeof(float) * vertex_count * 3;
        if (mesh->positions == NULL)
        {
            printf("Allocation of %d vertices\n", vertex_count);
            mesh->positions = (float*)AllocateGPUMemory(bytes);
        }
    }

    if ((attribute_bitfield & AttributeNormal) != 0)
    {
        if (mesh->normals == NULL)
        {
            printf("Allocation of %d normals\n", vertex_count);
            mesh->normals = (float*)AllocateGPUMemory(sizeof(float) * vertex_count * 3);
        }
    }

    if ((attribute_bitfield & AttributeTexcoord) != 0)
    {
        if (mesh->texcoords == NULL)
        {
            printf("Allocation of %d uvs\n", vertex_count);
            mesh->texcoords = (float*)AllocateGPUMemory(sizeof(float) * vertex_count * 2);
        }
    }

    mesh->allocated_vertex_count = M_MAX(vertex_count, mesh->allocated_vertex_count);
    mesh->vertex_count = vertex_count;
}

static void Setup_Arrays(struct Mesh* mesh)
{
	glEnableClientState(GL_VERTEX_ARRAY);
	glVertexPointer(3, GL_FLOAT, 0, mesh->positions);
    if(mesh->normals != NULL)
    {
        glEnableClientState(GL_NORMAL_ARRAY);
        glNormalPointer(GL_FLOAT, 0, mesh->normals);
        //screenprint("Mesh uses normals");
    }
    if (mesh->texcoords != NULL && (mesh->enabled_attributes & AttributeTexcoord) != 0)
    {
        glEnableClientState(GL_TEXTURE_COORD_ARRAY);
        glTexCoordPointer(2, GL_FLOAT, 0, mesh->texcoords);
        //screenprint("Mesh uses texcoords");
    }

}
static void Disable_Arrays(struct Mesh* mesh)
{
    if(mesh->normals != NULL)
    {
        glDisableClientState(GL_NORMAL_ARRAY);
    }
    if (mesh->texcoords != NULL && (mesh->enabled_attributes & AttributeTexcoord) != 0)
    {
        glDisableClientState(GL_TEXTURE_COORD_ARRAY);
    }
    glDisableClientState(GL_VERTEX_ARRAY);
}

static void Mesh_DrawLines(struct Mesh* mesh)
{
    if (mesh->indices != NULL)
    {
        for (int i = 0; i < mesh->index_count; i += 3)
        {
            glBegin(GL_LINE_LOOP);
            int v1 = mesh->indices[i+0] * 3;
            int v2 = mesh->indices[i+1] * 3;
            int v3 = mesh->indices[i+2] * 3;

            glVertex3f(mesh->positions[v1+0],
                    mesh->positions[v1+1],
                    mesh->positions[v1+2]);

            glVertex3f(mesh->positions[v2+0],
                    mesh->positions[v2+1],
                    mesh->positions[v2+2]);

            glVertex3f(mesh->positions[v3+0],
                    mesh->positions[v3+1],
                    mesh->positions[v3+2]);
            /*
            v = i * 3; // 0 - 2
            glVertex3f(mesh->positions[v+0],
                    mesh->positions[v+1],
                    mesh->positions[v+2]);
                    */
            glEnd();
        }

    }
    else
    {
        for (int i = 0; i < mesh->vertex_count; i += 3)
        {
            glBegin(GL_LINE_LOOP);
            int v = i * 3;  // 0 - 2
            glVertex3f(mesh->positions[v+0],
                    mesh->positions[v+1],
                    mesh->positions[v+2]);
            v += 3;  // 3 - 5
            glVertex3f(mesh->positions[v+0],
                    mesh->positions[v+1],
                    mesh->positions[v+2]);
            v += 3;  // 6 - 8
            glVertex3f(mesh->positions[v+0],
                    mesh->positions[v+1],
                    mesh->positions[v+2]);
            /*
            v = i * 3; // 0 - 2
            glVertex3f(mesh->positions[v+0],
                    mesh->positions[v+1],
                    mesh->positions[v+2]);
                    */
            glEnd();
        }
    }

}

static int Calculate_Percentage(int vertex_count, int percentage)
{
    if (percentage >= 100)
    {
        return vertex_count;
    }
    // How many triangles to draw
    int triangles = vertex_count/3;
    int p_amount = floor(((float)percentage/100.0f) * (float)triangles);
    int amount = M_MIN(triangles, p_amount);
    int draw_amount = M_MAX(0, amount);
    // Back to vertices
    return draw_amount * 3;
}


static void Mesh_DrawElements(struct Mesh* mesh, int percentage)
{
    Setup_Arrays(mesh);
    int draw_amount = Calculate_Percentage(mesh->vertex_count, percentage);
	glDrawElements(GL_TRIANGLES, draw_amount, GL_UNSIGNED_SHORT, mesh->indices);
    Disable_Arrays(mesh);

}

static void Mesh_DrawArrays(struct Mesh* mesh, int percentage)
{
    Setup_Arrays(mesh);
    int draw_amount = Calculate_Percentage(mesh->vertex_count, percentage);
    glDrawArrays(GL_TRIANGLES, 0, draw_amount);
    Disable_Arrays(mesh);
}

void Mesh_Draw(struct Mesh* mesh, enum MeshDrawMode mode)
{
    if (mode == DrawLines)
    {
        Mesh_DrawLines(mesh);
    }
    else
    {
        if (mesh->indices != NULL && mesh->index_count > 0)
        {
            Mesh_DrawElements(mesh, 100);
        }
        else
        {
            Mesh_DrawArrays(mesh, 100);
        }
    }
}

void Mesh_DrawPartial(struct Mesh* mesh, enum MeshDrawMode mode, int percentage)
{
    if (mode == DrawLines)
    {
        Mesh_DrawLines(mesh);
    }
    else
    {
        if (mesh->indices != NULL && mesh->index_count > 0)
        {
            Mesh_DrawElements(mesh, percentage);
        }
        else
        {
            Mesh_DrawArrays(mesh, percentage);
        }
    }

}

void Mesh_PrintInfo(struct Mesh* mesh, bool to_screen)
{
    if (mesh->positions != NULL)
    {
        if (to_screen)
        screenprintf("Mesh has %d positions\n", mesh->vertex_count);
        else
        printf("Mesh has %d positions\n", mesh->vertex_count);
    }
    if (mesh->normals != NULL)
    {
        if (to_screen)
        screenprintf("Mesh has %d normals\n", mesh->vertex_count);
        else
        printf("Mesh has %d normals\n", mesh->vertex_count);
    }
    if (mesh->texcoords != NULL)
    {
        if (to_screen)
        screenprintf("Mesh has %d texcoords\n", mesh->vertex_count);
        else
        printf("Mesh has %d texcoords\n", mesh->vertex_count);
    }
    if (mesh->indices != NULL)
    {
        if (to_screen)
        screenprintf("Mesh has %d indices\n", mesh->index_count);
        else
        printf("Mesh has %d indices\n", mesh->index_count);
    }
}

void Mesh_DisableAttribute(struct Mesh* mesh, enum VertexAttribute attrib)
{
    mesh->enabled_attributes = (mesh->enabled_attributes & ~attrib);
}
void Mesh_EnableAttribute(struct Mesh* mesh, enum VertexAttribute attrib)
{
    mesh->enabled_attributes = (mesh->enabled_attributes | attrib);
}

static float3 reflect_3d(float3 N, float3 source)
{
    float3 dest;
    float dot = M_DOT3(N, source);
    float3 reflect;
    M_SCALE3(reflect, N, (dot*2.0f));
    M_SUB3(dest, source, reflect);
    return dest;
}

void Mesh_GenerateMatcapUVs(struct Mesh* mesh)
{
    if (mesh->texcoords == NULL)
    {
        printf("allocated uvs");
        Mesh_Allocate(mesh, mesh->vertex_count, (AttributeTexcoord));
    }

    float modelView[16] = M_MAT4_IDENTITY();
    bool transpose = false;
    {
        if (transpose)
        {
            float modelViewMatrix[16];
            glGetFloatv(GL_MODELVIEW_MATRIX, modelViewMatrix);
            m_mat4_transpose(modelView, modelViewMatrix);

           /*
            screenprint("ModelviewMatrix");
            screenprintf("[%-0.2f %+.2f %+.2f %+.2f]", modelViewMatrix[0], modelViewMatrix[1], modelViewMatrix[2],modelViewMatrix[3] );
            screenprintf("[%-0.2f %+.2f %+.2f %+.2f]", modelViewMatrix[4], modelViewMatrix[5], modelViewMatrix[6],modelViewMatrix[7] );
            screenprintf("[%-0.2f %+.2f %+.2f %+.2f]", modelViewMatrix[8], modelViewMatrix[9], modelViewMatrix[10],modelViewMatrix[11] );
            screenprintf("[%-.2f %+.2f %+.2f %+.2f]", modelViewMatrix[12], modelViewMatrix[13], modelViewMatrix[14],modelViewMatrix[15] );
            */
        }
        else
        {
            glGetFloatv(GL_MODELVIEW_MATRIX, modelView);
        }
    }
    // Copy the values

    // Different ways
    // Normal
    // transposed

    /*
    for (int row = 0; row < 4; row++)
    {
        for (int col = 0; col < 4; col++)
        {
            modelView[row*4+col] = modelViewMatrix[row * 4 + col];
        }
    }
    */
    /*
    screenprint("Modelview");
    screenprintf("[%-0.2f %+.2f %+.2f %+.2f]", modelView[0], modelView[1], modelView[2],modelView[3] );
    screenprintf("[%-0.2f %+.2f %+.2f %+.2f]", modelView[4], modelView[5], modelView[6],modelView[7] );
    screenprintf("[%-0.2f %+.2f %+.2f %+.2f]", modelView[8], modelView[9], modelView[10],modelView[11] );
    screenprintf("[%-.2f %+.2f %+.2f %+.2f]", modelView[12], modelView[13], modelView[14],modelView[15] );
    */

    float inverseView[16] = M_MAT4_IDENTITY();
    float normalMatrix[16] = M_MAT4_IDENTITY();

    m_mat4_inverse(inverseView, modelView);
    m_mat4_transpose(normalMatrix, inverseView);

    // Generation

	float3 eye;
	float3 normal; // screen space normalo
	float4 normal4;
	float3 reflection;
	float2 R2;
	float3 position;
	float4 position4;
	float2 matcapUV;


	// Overwrite UVs
#define VERTICES_PER_FRAME 8092

	const float2 half = {0.5f, 0.5f};

    /*
    static int update_counter = 0;
    int target = (update_counter + VERTICES_PER_FRAME);
    bool restart_counter = false;
    if (target >= mesh->vertex_count)
    {
        target = mesh->vertex_count;
        restart_counter = true;
    }
    */

    int update_counter = 0;
    int target = mesh->vertex_count;
    int v = 0;
    int uv = 0;

    //printf("Target %d\n", target);
    //printf("writing %d uvs\n", mesh->vertex_count);
	for (int i = update_counter; i < target; i++)
	{
        v = i * 3;
        uv = i * 2; // DANGER This is important distinction

		position.x = mesh->positions[v+0];
		position.y = mesh->positions[v+1];
		position.z = mesh->positions[v+2];

		normal.x = mesh->normals[v+0];
		normal.y = mesh->normals[v+1];
		normal.z = mesh->normals[v+2];

		normal4.x = normal.x;
        normal4.y = normal.y;
        normal4.z = normal.z;
        normal4.w = 0.0f;

		position4.x = position.x;
        position4.y = position.y;
        position4.z = position.z;
        position4.w = 1.0f;

		m_mat4_transform4(&position4, modelView, &position4);

        position.x = position4.x;
        position.y = position4.y;
        position.z = position4.z;

		matcapUV.x = 0.5f;
        matcapUV.y = 0.5f;
		if (M_LENGHT3(position) != 0.0f)
		{
			M_NORMALIZE3(eye, position);
            m_mat4_transform4(&normal4, normalMatrix, &normal4);
            normal.x = normal4.x;
            normal.y = normal4.y;
            normal.z = normal4.z;
			M_NORMALIZE3(normal, normal);

            reflection = reflect_3d(normal, eye); // Reflect eye with normal

			const float rx2 = pow(reflection.x, 2.0f);
			const float ry2 = pow(reflection.y, 2.0f);
			const float rz12 = pow(reflection.z+1, 2.0f);
			const float sqrtR2 = sqrt(rx2 + ry2 + rz12) * 2.0f;
            if (sqrtR2 != 0.0f)
            {
                R2.x = reflection.x/sqrtR2;
                R2.y = reflection.y/sqrtR2;
                M_ADD2(matcapUV, R2, half);
            }
		}
		mesh->texcoords[uv+0] =  matcapUV.x;
		mesh->texcoords[uv+1] =  1.0f - matcapUV.y;
	}
	FlushGPUCache(mesh->texcoords, mesh->vertex_count * sizeof(float) * 2);

    /*
    if (restart_counter)
    {
        update_counter = 0;
    }
    else
    {
        update_counter = target;
    }
    */
}

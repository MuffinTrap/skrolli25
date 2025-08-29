#ifndef KOCH_FLAKE_C
#define KOCH_FLAKE_C

#include "koch_flake.h"
#include <math.h>
#include <m_float2_math.h>
#include <opengl_include.h>
#include "../Ziz/screenprint.h"

/**
 * @brief Rotate a 2D vector around another point
 */
static float2 rotate_z_point(float2 vector, float radians, float2 point)
{
    float2 temp = {vector.x-point.x, vector.y - point.y};
    temp = M_ROTATE2(temp, radians);
    float2 rotated = {temp.x + point.x, temp.y + point.y};
    return rotated;
}

static float2 lerp_normalized(float2 A, float2 B, float t)
{
    float2 AB;
    M_SUB2(AB, B, A);
    float length = M_LENGHT2(AB);
    float2 dir;
    M_NORMALIZE2(dir, AB);
    M_SCALE2(dir, dir, (length*t));
    M_ADD2(dir, A, dir);
    return dir;
}

PointList* subtriangle(float2 A, float2 B, float angle, float ratio, float extrusion, PointList* list)
{
    float2 m1 = lerp_normalized(A, B, ratio);
    float2 m2 = lerp_normalized(A, B, 1.0f - ratio);
    float2 m3 = lerp_normalized(m1, m2, extrusion);
    m3 = rotate_z_point(m3, angle * M_DEG_TO_RAD, m1);

    PointList_push_point(list, m1);
    PointList_push_point(list, m2);
    PointList_push_point(list, m3);
    return list;
}

/**
 * @brief Send 3 vertices to OpenGL
 */
#define TRIANGLE_DRAW(array) glVertex2f(array[0].x, array[0].y); glVertex2f(array[1].x, array[1].y); glVertex2f(array[2].x, array[2].y);



void koch_line(float2 A, float2 B, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list )
{
    //printf("\tKoch line %d\n", recursion);
    if (recursion == 0 )
    {
       // PointList_push_point(recursive_list, A);
    }
    else
    {
        const short next_level = recursion - 1;

        int first_new = recursive_list->used_size;
        recursive_list = subtriangle(A, B, angle, ratio, extrusion, recursive_list);
        //PointList_glVertex2f(recursive_list);

        float2 triangle_0 = recursive_list->points[first_new + 0];
        float2 triangle_1 = recursive_list->points[first_new + 1];
        float2 triangle_2 = recursive_list->points[first_new + 2];

        koch_line(A, triangle_0,            next_level, angle, ratio, extrusion, recursive_list);
        koch_line(triangle_0, triangle_2,   next_level, angle, ratio, extrusion, recursive_list);
        koch_line(triangle_2, triangle_1,   next_level, angle, ratio, extrusion, recursive_list);
        koch_line(triangle_1, B,            next_level, angle, ratio, extrusion, recursive_list);
    }
    //PointList_push_point(recursive_list, B);
}

void draw_koch(float2 A, float2 B, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list)
{
    //printf("\tdraw_koch recursion %d\n", recursion);
    koch_line(A, B, recursion, angle, ratio, extrusion, recursive_list);
}


PointList* get_corners(float2 center, short amount, float radius, float angle, PointList* list)
{
    PointList_reserve(list, (size_t)amount);

    float corners = (float)amount;
    float step = (M_PI * 2.0f) / corners;
    float radians = angle * M_DEG_TO_RAD;
    float current_angle = radians;
    float2 start = {0, radius};
    float2 point = {0,0};
    float2 rotated = {0,0};
    for (short i = 0; i < amount; i++)
    {
        rotated = M_ROTATE2(start, current_angle);
        M_ADD2(point, rotated, center);
        PointList_set_point(list, point, i);
        current_angle += step;
    }
    return list;
}

void store_snowflake_struct(KochFlake* flake)
{
    store_snowflake(flake->center, flake->radius, flake->recursion_level, flake->angle, flake->ratio, flake->extrusion, &flake->recursive_list);
}

void store_snowflake(float2 center, float radius, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list)
{
    if (recursion < 0)
    {
        return;
    }
    PointList_clear(recursive_list);

    recursive_list = get_corners(center, 3, radius, angle, recursive_list);
    if (recursion > 0)
    {
        draw_koch(recursive_list->points[2], recursive_list->points[1], recursion, angle, ratio, extrusion, recursive_list);
        draw_koch(recursive_list->points[1], recursive_list->points[0], recursion, angle, ratio, extrusion, recursive_list);
        draw_koch(recursive_list->points[0],
                recursive_list->points[2], recursion, angle, ratio, extrusion, recursive_list);
    }
}

void draw_snowflake_struct(KochFlake* flake)
{
    draw_snowflake(flake->center, flake->radius, flake->recursion_level, flake->angle, flake->ratio, flake->extrusion, &flake->recursive_list);

}

void draw_snowflake(float2 center, float radius, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list)
{
    store_snowflake(center, radius, recursion, angle, ratio, extrusion, recursive_list);
    glBegin(GL_TRIANGLES);
    PointList_glVertex2f(recursive_list);
    screenprintf("Koch flake: %d vertices", recursive_list->used_size);

    glEnd();
}

void KochFlake_WriteToMesh(struct KochFlake* flake, struct Mesh* mesh)
{
    store_snowflake_struct(flake);
    int vertices = flake->recursive_list.used_size;
    Mesh_Allocate(mesh, vertices, (AttributePosition));
    if (mesh->positions == NULL)
    {
        printf("Mesh not allocated!");
        return;
    }
    for(int i = 0; i < vertices; i++)
    {
    //printf("Writing flake to mesh: %d vertices in list\n", vertices);
        float2 p = flake->recursive_list.points[i];
        int float_index = i * 3;
        mesh->positions[float_index+0] = p.x;
        mesh->positions[float_index+1] = p.y;
        mesh->positions[float_index+2] = 0.0f;
    }
}

struct KochFlake KochFlake_CreateDefault(short recursion_level)
{
    KochFlake flake;
    int needed_size = 3;
    switch(recursion_level)
    {
        case 0:
        needed_size = 3;
        break;
        case 1:
            needed_size = 12;
            break;
        case 2:
            needed_size = 48;
            break;
        case 3:
            needed_size = 192;
            break;
        case 4:
            needed_size = 768;
            break;
        case 5:
            needed_size = 3072;
            break;
    }

	flake.recursive_list = PointList_create(needed_size);

    float2 zero = {0.0f, 0.0f};
    flake.center = zero;
    flake.radius = 1.0f;
    flake.recursion_level = 5;
    flake.angle = 60;
    flake.ratio = 1.0f/3.0f;
    flake.extrusion = 1.0f;
    flake.recursion_level = 4;

    return flake;
}

#endif

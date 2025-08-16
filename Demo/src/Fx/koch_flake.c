#ifndef KOCH_FLAKE_C
#define KOCH_FLAKE_C

#include "koch_flake.h"
#include <math.h>

// float2 functions
/**
 * @brief Rotate a 2D vector around origo
 */
float2 rotate_z(float2 vector, float radians)
{
    float c = cos(radians);
    float s = sin(radians);
    float x = vector.x * c - vector.y * s;
    float y = vector.x * s + vector.y * c;
    float2 rotated;
    rotated.x = x;
    rotated.y = y;
    return rotated;
}

/**
 * @brief Rotate a 2D vector around another point
 */
static float2 rotate_z_point(float2 vector, float radians, float2 point)
{
    float2 temp = {vector.x-point.x, vector.y - point.y};
    temp = rotate_z(temp, radians);
    float2 rotated = {temp.x + point.x, temp.y + point.y};
    return rotated;
}

static float2 direction_2d(float2 A, float2 B)
{
    float2 dir;
    M_SUB2(dir, B, A);
    M_NORMALIZE2(dir, dir);
    return dir;
}

#define M_SCALE2(dest, A, scalar) {(dest).x = (A).x * scalar; (dest).y = (A).y * scalar;}

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

    PointList_set_point(list, m1, 0);
    PointList_set_point(list, m2, 1);
    PointList_set_point(list, m3, 2);
    return list;
}

/**
 * @brief Send 3 vertices to OpenGL
 */
#define TRIANGLE_DRAW(array) glVertex2f(array[0].x, array[0].y); glVertex2f(array[1].x, array[1].y); glVertex2f(array[2].x, array[2].y);



void koch_line(float2 A, float2 B, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list, PointList* local_list)
{
    //printf("\tKoch line %d\n", recursion);
    if (recursion == 0 )
    {
        PointList_push_point(recursive_list, A);
    }
    else
    {
        const short next_level = recursion - 1;

        local_list = subtriangle(A, B, angle, ratio, extrusion, local_list);
        PointList_glVertex2f(local_list);

        float2 triangle_0 = local_list->points[0];
        float2 triangle_1 = local_list->points[1];
        float2 triangle_2 = local_list->points[2];

        koch_line(A, triangle_0,            next_level, angle, ratio, extrusion, recursive_list, local_list);
        koch_line(triangle_0, triangle_2,   next_level, angle, ratio, extrusion, recursive_list, local_list);
        koch_line(triangle_2, triangle_1,   next_level, angle, ratio, extrusion, recursive_list, local_list);
        koch_line(triangle_1, B,            next_level, angle, ratio, extrusion, recursive_list, local_list);
    }
    PointList_push_point(recursive_list, B);
}

void draw_koch(float2 A, float2 B, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list, PointList* local_list)
{
    //printf("\tdraw_koch recursion %d\n", recursion);
    koch_line(A, B, recursion, angle, ratio, extrusion, recursive_list, local_list);
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
        rotated = rotate_z(start, current_angle);
        M_ADD2(point, rotated, center);
        PointList_set_point(list, point, i);
        current_angle -= step;
    }
    return list;
}


void draw_snowflake_struct(KochFlake* flake)
{
    draw_snowflake(flake->center, flake->radius, flake->recursion_level, flake->angle, flake->ratio, flake->extrusion, &flake->recursive_list, &flake->local_list);

}

void draw_snowflake(float2 center, float radius, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list, PointList* local_list)
{
    if (recursion < 0)
    {
        return;
    }
    PointList_clear(recursive_list);
    PointList_clear(local_list);

    glBegin(GL_TRIANGLES);

    recursive_list = get_corners(center, 3, radius, angle, recursive_list);
    // NOTE The first triangle is wrong way around
    for (int i = recursive_list->used_size-1 ; i >= 0; i--)
    {
        glVertex2f(recursive_list->points[i].x, recursive_list->points[i].y);
    }
    if (recursion > 0)
    {
        const int last = recursive_list->used_size - 1;
        //printf("Draw snowflake until %d\n", last);
        for (int p = 0; p < last; p++)
        {
            //printf("Flake point %d/%d\n", p, last);
            draw_koch(recursive_list->points[p], recursive_list->points[p+1], recursion, angle, ratio, extrusion, recursive_list, local_list);
        }
        draw_koch(recursive_list->points[recursive_list->used_size-1],
                recursive_list->points[0], recursion, angle, ratio, extrusion, recursive_list, local_list);
    }


    glEnd();
}


#endif

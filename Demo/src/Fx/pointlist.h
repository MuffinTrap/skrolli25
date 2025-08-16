#ifndef POINTLIST_H
#define POINTLIST_H

#include <m_math.h>
#include "../Ziz/opengl_include.h"

struct PointList
{
    float2* points;
    size_t allocated_size;
    size_t used_size;
};
typedef struct PointList PointList;

PointList PointList_create(size_t size)
{
    PointList list;
    list.points = (float2*)malloc(sizeof(float2) * size);
    list.allocated_size = size;
    list.used_size = 0;
    return list;
}

/**
 * @brief Ensure that list can hold at least new_size points
 * @param list The list
 * @param new_size New minimum size
 */
void PointList_reserve(PointList* list, size_t new_size)
{
    if (list->allocated_size < new_size)
    {
        list->points = (float2*)realloc(list->points, sizeof(float2) * new_size);
        list->allocated_size = new_size;
    }
}

void PointList_clear(PointList* list)
{
    list->used_size = 0;
}

void PointList_set_point(PointList* list, float2 point, size_t index)
{
    if (index < list->allocated_size)
    {
        list->points[index] = point;
    }
}

void PointList_push_point(PointList* list, float2 point)
{
    if (list->used_size >= list->allocated_size)
    {
        list->points = (float2*)realloc(list->points, sizeof(float2) * list->allocated_size * 2);
        list->allocated_size *= 2;
    }

    if (list->used_size < list->allocated_size)
    {
        list->points[list->used_size + 1] = point;
        list->used_size += 1;
    }
}

/**
 * @brief Send the points to OpenGL
 */
void PointList_glVertex2f(PointList* list)
{
    for (size_t i = 0; i < list->used_size; i++)
    {
        glVertex2f(list->points[i].x, list->points[i].y);
    }
}

#endif

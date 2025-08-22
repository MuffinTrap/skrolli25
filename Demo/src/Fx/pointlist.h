#ifndef POINTLIST_H
#define POINTLIST_H

#include <m_math.h>
#include <opengl_include.h>

#define POINT_LIST_MAX_SIZE 4096

struct PointList
{
    float2* points;
    int allocated_size;
    int used_size;
};
typedef struct PointList PointList;

PointList PointList_create(int size)
{
    PointList list;
    list.points = (float2*)malloc( sizeof(float2) * size );
    list.allocated_size = size;
    list.used_size = 0;
    return list;
}

/**
 * @brief Ensure that list can hold at least new_size points
 * @param list The list
 * @param new_size New minimum size
 */
void PointList_reserve(PointList* list, int new_size)
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

void PointList_set_point(PointList* list, float2 point, int index)
{
    if (index < list->allocated_size)
    {
        list->points[index] = point;
        list->used_size = M_MAX(index+1, list->used_size);
    }
}

void PointList_push_point(PointList* list, float2 point)
{
    if (list->used_size >= list->allocated_size)
    {
        int new_size = list->allocated_size * 2;
        if (new_size > POINT_LIST_MAX_SIZE)
        {
            new_size = POINT_LIST_MAX_SIZE;
        }
        if (new_size <= POINT_LIST_MAX_SIZE)
        {
            list->points = (float2*)realloc(list->points, sizeof(float2) * new_size);
            list->allocated_size *= 2;
        }
    }

    if (list->used_size < list->allocated_size)
    {
        list->points[list->used_size] = point;
        list->used_size += 1;
    }
}

/**
 * @brief Send the points to OpenGL
 */
void PointList_glVertex2f(PointList* list)
{
    for (int i = 0; i < list->used_size; i++)
    {
        glVertex2f(list->points[i].x, list->points[i].y);
    }
}

#endif

#include "pointlist.h"
#include <opengl_include.h>

PointList PointList_create(int size)
{
    PointList list;
    list.points = (float2*)malloc( sizeof(float2) * size );
    list.allocated_size = size;
    list.used_size = 0;
    return list;
}


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

float2 PointList_get_last(PointList* list)
{
    if (list->used_size > 0)
    {
        return list->points[list->used_size-1];
    }
    else
    {
        float2 zero = {0.0f, 0.0f};
        return zero;
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

void PointList_glVertex2f(PointList* list)
{
    for (int i = 0; i < list->used_size; i++)
    {
        glVertex2f(list->points[i].x, list->points[i].y);
    }
}

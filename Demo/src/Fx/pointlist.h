#ifndef POINTLIST_H
#define POINTLIST_H

#include <m_math.h>

#define POINT_LIST_MAX_SIZE 4096

struct PointList
{
    float2* points;
    int allocated_size;
    int used_size;
};
typedef struct PointList PointList;

PointList PointList_create(int size);

/**
 * @brief Ensure that list can hold at least new_size points
 * @param list The list
 * @param new_size New minimum size
 */
void PointList_reserve(PointList* list, int new_size);

void PointList_clear(PointList* list);

void PointList_set_point(PointList* list, float2 point, int index);

float2 PointList_get_last(PointList* list);

void PointList_push_point(PointList* list, float2 point);

/**
 * @brief Send the points to OpenGL
 */
void PointList_glVertex2f(PointList* list);

#endif

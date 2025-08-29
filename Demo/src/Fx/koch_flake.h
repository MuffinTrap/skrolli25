#ifndef KOCH_FLAKE_H
#define KOCH_FLAKE_H

#include <m_math.h>
#include <stdbool.h>
#include "pointlist.h"
#include "../Ziz/mesh.h"

struct KochFlake
{
    float2 center;
    float radius;
    float ratio;
    float angle;
    float extrusion;
    short recursion_level;
    PointList recursive_list;
};
typedef struct KochFlake KochFlake;

struct KochFlake KochFlake_CreateDefault(short recursion_level);

void KochFlake_WriteToMesh(struct KochFlake* flake, struct Mesh* mesh);

/**
 * @brief Recursively calculate the points on the line and draw the triangle
 * @param A Point A of line
 * @param B Poitn B of line
 * @param recursion Level of recursion. Recursion stops when this is < 0
 * @param angle Angle in degrees
 * @param ratio Ratio of the shape
 * @param extrusion extrusion of the shape
 * @param recursive_list The PointList to use to collect all points
 * @param local_list The PointList to use to draw local triangles
 */
void koch_line(float2 A, float2 B, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list);

/**
 * @brief Calculate and draw a Koch curve between points A and B
 * @param A Point A
 * @param B Point B
 * @param recursion How many levels of recursion
 * @param angle Angle of the shape in degrees
 * @param extrusion extrusion of the shape
 * @param recursive_list The PointList to use to collect all points
 * @param local_list The PointList to use to draw local triangles
 */
void draw_koch(float2 A, float2 B, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list);

/**
 * @brief Draw a koch snowflake around a center point
 * @param center Center point of shape
 * @param radius Radius of the flake
 * @param recursion Recursion level of the shape: minimum 0
 * @param angle Rotation of the shape in degrees
 * @param recursive_list The PointList to use to collect all points
 * @param display Is the result drawn immediately?
 */
void draw_snowflake(float2 center, float radius, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list);

void draw_snowflake_struct(KochFlake* flake);

void store_snowflake_struct(KochFlake* flake);
void store_snowflake(float2 center, float radius, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list);

/**
 * @brief Break the line from A to B up by ratio and create third point
 * @param A Point A
 * @param B Point B
 * @param angle Angle from AB to third point in degrees
 * @param ratio Normalized distance from A to B for the third point
 * @param extrusion extrusion?
 * @param list PointList to write the points to
 * @return List containing new triangle points
 */
PointList* subtriangle(float2 A, float2 B, float angle, float ratio, float extrusion, PointList* list);

/**
 * @brief Generate a list of points around a center point
 * @param center Center of the polygon
 * @param amount How many points to generate
 * @param radius Radius of the polygon
 * @param angle Initial rotation of the polygon in degrees
 * @return Array of corners
 */
void get_corners(float2 center, short amount, float radius, float angle, float2* points);

#endif

#include "gosper_curve.h"
#include <m_float2_math.h>
#include <stdio.h>
#include <opengl_include.h>
#include "../Ziz/screenprint.h"

static float2 latest; // Middle point of the path
static float2 direction;
static float2 to_right;
static float2 left; // Left side of path
static float2 right; // Right side of path
static float step;
static float half_width;
static const float radians = 60.0f * M_DEG_TO_RAD;

static void Gosper_Turn(float sign, struct PointList* points)
{
    // Rotate direction
    M_ROTATE2_PTR(&direction, sign * radians);
    // calculate right
    M_RIGHT_ANGLE2(to_right, direction);
    M_SCALE2(to_right, to_right, (half_width));

    // store left or right depending on sign
    // store new point as left or right
    // and update middle point
    if (sign > 0)
    {
        PointList_push_point(points, left);
        M_ADD2(latest, left, to_right);
        M_ADD2(right, latest, to_right);
        PointList_push_point(points, right);
    }
    else
    {
        M_SUB2(latest, right, to_right);
        M_SUB2(left, latest, to_right);
        PointList_push_point(points, left);
        PointList_push_point(points, right);
    }
}


void Gosper_Move(struct PointList* points)
{
    float2 next;
    M_SCALE2(next, direction, step);
    M_ADD2(next, next, latest);
    latest = next;
    M_SUB2(left, next, to_right);
    M_ADD2(right, next, to_right);

    PointList_push_point(points, left);
    PointList_push_point(points, right);

    //printf("Added %.2f, %.2f\n", latest.x, latest.y);
}

void Gosper_A(struct PointList* points, short recursion_level)
{
    if (recursion_level <= 0)
    {
        Gosper_Move(points);
        return;
    }

    const short next_recursion = recursion_level - 1;
    Gosper_A(points, next_recursion);
    Gosper_Turn(-1.0f, points);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f, points);
    Gosper_Turn(-1.0f, points);
    Gosper_B(points, next_recursion);
    Gosper_Turn(1.0f, points);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f, points);
    Gosper_Turn(1.0f, points);
    Gosper_A(points, next_recursion);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f, points);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f, points);
}

void Gosper_B(struct PointList* points, short recursion_level)
{
    if (recursion_level <= 0)
    {
        Gosper_Move(points);
        return;
    }

    const short next_recursion = recursion_level - 1;
    Gosper_Turn(1.0f, points);
    Gosper_A(points, next_recursion);
    Gosper_Turn(-1.0f, points);
    Gosper_B(points, next_recursion);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f, points);
    Gosper_Turn(-1.0f, points);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f, points);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f, points);
    Gosper_Turn(1.0f, points);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f, points);
    Gosper_B(points, next_recursion);
}



void Gosper_Create(struct PointList* points, float2 start, float2 start_dir, float step_length, float path_width, short recursion_level)
{
    PointList_clear(points);
    latest = start;
    direction = start_dir;
    step = step_length;
    half_width = path_width / 2.0f;
    M_RIGHT_ANGLE2(to_right, direction);
    M_SCALE2(to_right, to_right, half_width);

    M_SUB2(left, latest, to_right);
    M_ADD2(right, latest, to_right);
    PointList_push_point(points, left);
    PointList_push_point(points, right);

    Gosper_A(points, recursion_level);
    //printf("Created gosper curve with %d points\n", points->used_size);
}

float2 Gosper_Draw(struct PointList* list, struct Gradient* gradient, float amount, float gradient_offset)
{
    // TODO use quads
    // store to list when there is a turn
    // or store left and right vertex to list
    float2 left;
    float2 right;
    glBegin(GL_QUAD_STRIP);
    // Last is always odd number
    int last_index = M_MIN( list->used_size, (int)floor(amount) * 2 -1);
    float gradient_step = 1.0f / (float)list->used_size;
    float gradient_stop = gradient_offset;

    for(int i = 0; i < last_index; i += 2)
    {
        left = list->points[i];
        right = list->points[i+1];
        Gradient_glColor(gradient, gradient_stop);
        glVertex2f(left.x, left.y);
        glVertex2f(right.x, right.y);
        gradient_stop += gradient_step;
    }

    // Draw last partial segment
    int end_point_left = M_MIN( list->used_size-2, last_index+1);
    int end_point_right = M_MIN( list->used_size-1, last_index+2);

    screenprintf("Last index %d", last_index);
    screenprintf("Last is %d/%d Amount %.2f\n", end_point_left, end_point_right, amount);

    float2 last_left = list->points[end_point_left];
    float2 last_right = list->points[end_point_right];
    if (last_index < list->used_size-1)
    {
        float partial = 1.0f;
        partial = amount - floor(amount);
        screenprintf("Partial %.2f\n", partial);
        float2 dir;
        M_SUB2(dir, last_left, left);
        dir.x = dir.x*partial;
        dir.y = dir.y*partial;
        screenprintf("dir is %.2f/%.2f\n", dir.x, dir.y);
        //M_NORMALIZE2(dir, dir);
        //M_SCALE2(dir, dir, partial * step);
        M_ADD2(last_left, left, dir);
        M_ADD2(last_right, right, dir);
    }
    glVertex2f(last_left.x, last_left.y);
    glVertex2f(last_right.x, last_right.y);

    screenprintf("draw from %.2f,%.2f t %.2f,%.2f\n", left.x, left.y, last_left.x, last_left.y);
    glEnd();

    float2 dir = direction_2d(last_left, last_right);
    M_SCALE2(dir, dir, half_width);
    float2 middle;
    M_ADD2(middle, last_right, dir);
    return middle;


}

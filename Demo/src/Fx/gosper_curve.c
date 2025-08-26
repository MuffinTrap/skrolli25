#include "gosper_curve.h"
#include <m_float2_math.h>

static float2 latest;
static float2 direction;
static float step;
static const radians = 60.0f * M_DEG_TO_RAD;

static void Gosper_Turn(float sign)
{
    // TODO create 2 points for turning
    M_ROTATE2_PTR(&direction, sign * radians);
}

void Gosper_Move(struct PointList* points)
{
    float2 next;
    M_SCALE2(next, direction, step);
    M_ADD2(next, next, latest);
    latest = next;
    PointList_push_point(points, next);

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
    Gosper_Turn(-1.0f);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f);
    Gosper_Turn(-1.0f);
    Gosper_B(points, next_recursion);
    Gosper_Turn(1.0f);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f);
    Gosper_Turn(1.0f);
    Gosper_A(points, next_recursion);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f);
}

void Gosper_B(struct PointList* points, short recursion_level)
{
    if (recursion_level <= 0)
    {
        Gosper_Move(points);
        return;
    }

    const short next_recursion = recursion_level - 1;
    Gosper_Turn(1.0f);
    Gosper_A(points, next_recursion);
    Gosper_Turn(-1.0f);
    Gosper_B(points, next_recursion);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f);
    Gosper_Turn(-1.0f);
    Gosper_B(points, next_recursion);
    Gosper_Turn(-1.0f);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f);
    Gosper_Turn(1.0f);
    Gosper_A(points, next_recursion);
    Gosper_Turn(1.0f);
    Gosper_B(points, next_recursion);
}



struct PointList Gosper_Create(float2 start, float2 start_dir, float step_length, short recursion_level)
{
    PointList points = PointList_create(128);
    PointList_push_point(&points, start);

    latest = start;
    direction = start_dir;
    step = step_length;

    Gosper_A(&points, recursion_level);
    printf("Created gosper curve with %d points\n", points.used_size);
    return points;
}

void Gosper_Draw(struct PointList* list, float amount)
{
    // TODO use quads
    // store to list when there is a turn
    // or store left and right vertex to list
    float2 A;
    glBegin(GL_LINE_STRIP);
    int last_index = M_MIN( list->used_size-1, (int)floor(amount));

    for(int i = 0; i < last_index; i++)
    {
        A = list->points[i];
        glVertex2f(A.x, A.y);
    }

    // Draw last partial segment
    int end_point = M_MIN( list->used_size-1, last_index);
    screenprintf("Last is %d/%d Amount %d\n", end_point, list->used_size-1, amount);

    float2 B = list->points[end_point];
    if (last_index < list->used_size-1)
    {
        const short amount_short = (short)amount;
        float partial = 1.0f;
        partial = amount - floor(amount);
        screenprintf("Partial %.2f\n", partial);
        float2 dir;
        M_SUB2(dir, B, A);
        dir.x = dir.x*partial;
        dir.y = dir.y*partial;
        screenprintf("dir is %.2f/%.2f\n", dir.x, dir.y);
        //M_NORMALIZE2(dir, dir);
        //M_SCALE2(dir, dir, partial * step);
        M_ADD2(B, A, dir);
    }
    glVertex2f(B.x, B.y);
    glEnd();
}

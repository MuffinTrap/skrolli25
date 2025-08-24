#include "gradient.h"
#include <opengl_include.h>

struct Gradient Gradient_CreateEmpty(void)
{
    struct Gradient g;
    g.color_amount = 0;
    return g;
}

void Gradient_PushColor(struct Gradient* gradient, color3* color, float stop)
{
    if (gradient->color_amount + 1 < GRADIENT_SIZE)
    {
        gradient->colors[gradient->color_amount] = color;
        gradient->stops[gradient->color_amount] = stop;
        gradient->color_amount++;
    }
}

static float lerp(float a, float b, float t)
{
    return a * (1.0f - t) + (b * t);
}

void Gradient_glColor(struct Gradient* gradient, float stop)
{
    color3 between = Gradient_GetColor(gradient, stop);
    glColor3f(between.r, between.g, between.b);
}
void Gradient_glColorA(struct Gradient* gradient, float stop, float alpha)
{
    color3 between = Gradient_GetColor(gradient, stop);
    glColor4f(between.r, between.g, between.b, alpha);
}

color3 Gradient_GetColor(struct Gradient* gradient, float stop)
{
    // NOTE must have at least 2 colors for this to work
    short before_index = 0;
    short after_index = gradient->color_amount;

    for (short i = 0; i < gradient->color_amount-1; i++)
    {
        if (stop >= gradient->stops[i])
        {
            before_index = i;
            after_index = i+1;
        }
    }
    if (after_index >= gradient->color_amount)
    {
        after_index = gradient->color_amount-1;
        before_index = after_index - 1;
    }
    color3* before = gradient->colors[before_index];
    color3* after = gradient->colors[after_index];
    float range = gradient->stops[after_index] - gradient->stops[before_index];
    float into_next = (stop - gradient->stops[before_index]);
    float t = into_next/range;

    color3 between;
    between.r = lerp(before->r, after->r, t);
    between.g = lerp(before->g, after->g, t);
    between.b = lerp(before->b, after->b, t);
    return between;
}

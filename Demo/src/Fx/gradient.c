#include "gradient.h"

#include "../Ziz/screenprint.h"
#include "color_manager.h"
#include <opengl_include.h>

struct Gradient Gradient_CreateEmpty(enum GradientShape shape, enum GradientLoopMode loop_mode)
{
    struct Gradient g;
    g.color_amount = 0;
    g.shape = shape;
    g.loop_mode = loop_mode;
    g.alpha = 1.0f;
    g.repeats = 1.0f;
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

void Gradient_PushName(struct Gradient* gradient, enum ColorName name, float stop)
{
    if (gradient->color_amount + 1 < GRADIENT_SIZE)
    {
        gradient->colors[gradient->color_amount] = ColorManager_GetName(name);
        gradient->stops[gradient->color_amount] = stop;
        gradient->color_amount++;
    }
}

void Gradient_PushColorArray(struct Gradient* gradient, enum ColorName* colors, short amount)
{
    if (amount < GRADIENT_SIZE)
    {
        float step = 1.0f/(float)amount;
        for (short i = 0; i < amount; i++)
        {
            gradient->colors[i] = ColorManager_GetName(colors[i]);
            gradient->stops[i] = step * i;
        }
        gradient->color_amount = amount;
    }
}

static float color_lerp(float a, float b, float t)
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
    float read_stop = 0.0f;

    if (gradient->repeats > 0.0f)
    {
        // When repeats is 2; the gradient has been travelled twice by the time stop reaches 1.0f
        // When repeats is 0.5; only half of the gradient has been travelled when stop reaches 1.0f
        // If repeats is 0, only the color at 0.0 is given
        // negative repeats work the same way
        stop = stop * gradient->repeats;
    }

    if (stop < 0.0f)
    {
        stop = stop * -1.0f;
    }

    if (stop > 1.0f)
    {
        // Going forward or backward?
        const float modulostop = floor(stop);
        const short modulo = ((short)modulostop) % 2;
                                    // 2.15 - 2.0 == 0.15
        const float decimal_part = (stop - floor(stop));
        //screenprintf("Decimal %f, Modulo %d", decimal_part, modulo);
        read_stop = decimal_part;
        if (gradient->loop_mode == GradientLoopRepeat)
        {
            // Change to 2.15 -> 0.15
            read_stop = decimal_part;
        }
        if (gradient->loop_mode == GradientLoopMirror)
        {
            if (modulo == 0)
            {
                // Change to 2.15 -> 0.15
                read_stop = decimal_part;
            }
            else
            {
                // Change to 1.15 -> 0.85
                read_stop = 1.0f - decimal_part;
            }
        }
    }
    else
    {
        read_stop = stop;
    }
    //screenprintf("Stop: %.4f, Read: %.4f", stop, read_stop);

    for (short i = 0; i < gradient->color_amount-1; i++)
    {
        if (read_stop >= gradient->stops[i])
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
    float into_next = (read_stop - gradient->stops[before_index]);
    float t = into_next/range;

    color3 between;
    between.r = color_lerp(before->r, after->r, t);
    between.g = color_lerp(before->g, after->g, t);
    between.b = color_lerp(before->b, after->b, t);
    return between;
}

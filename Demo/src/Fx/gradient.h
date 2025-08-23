#ifndef GRADIENT_H
#define GRADIENT_H
// Refers to colors in color manager and has 2 or more stops
#include "color_manager.h"

#define GRADIENT_SIZE 8
struct Gradient
{
    color3 colors[GRADIENT_SIZE];
    float stops[GRADIENT_SIZE];
    short color_amount;
};

struct Gradient Gradient_CreateEmpty(void);
void Gradient_PushColor(struct Gradient* gradient, color3 color, float stop);
void Gradient_glColor(struct Gradient* gradient, float stop);
color3 Gradient_GetColor(struct Gradient* gradient, float stop);

#endif

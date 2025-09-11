#ifndef GOSPER_CURVE_H
#define GOSPER_CURVE_H

#include <m_math.h>
#include "pointlist.h"
#include "gradient.h"

void Gosper_Create(struct PointList* list, float2 start, float2 start_dir, float step_length, float path_width, short recursion_level);


/**
 * @brief Draws the curve.
 * @param points The points of curve
 * @param amount How many segments to draw. 3.5 draws 3 full and a half segment
 * @return The point that was drawn last
 */
float2 Gosper_Draw(struct PointList* points, struct Gradient* gradient, float amount, float gradient_offset);

void Gosper_A(struct PointList* points, short recursion_level);
void Gosper_B(struct PointList* points, short recursion_level);
#endif

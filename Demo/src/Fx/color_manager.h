#ifndef COLOR_MANAGER_H
#define COLOR_MANAGER_H
// Contains all colors, can read color from hex
#include <m_math.h>

struct color3
{
    float r;
    float g;
    float b;
};
typedef struct color3 color3;

enum ColorName
{
    ColorWhite,
    ColorBlack,

    ColorRose,
    ColorPurple,
    ColorCyanblue
};

void ColorManager_LoadColors(void);
color3* ColorManager_Get(short index);
color3* ColorManager_GetName(enum ColorName name);
color3 ColorManager_HexToColor(unsigned int hex);

#endif

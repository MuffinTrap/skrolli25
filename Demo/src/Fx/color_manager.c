#include "color_manager.h"

#define RED(c)		(((c)>>24)&0xFF)	/*!< Gets the red component intensity from a 32-bit color value.
										 *	 \param[in] c 32-bit RGBA color value
										 *	 \return Red component of value
										 */

#define GREEN(c)	(((c)>>16)&0xFF)	/*!< Gets the green component intensity from a 32-bit color value.
										 *	 \param[in] c 32-bit RGBA color value
										 *	 \return Green component of value
										 */

#define BLUE(c) 	(((c)>>8)&0xFF)		/*!< Gets the blue component intensity from a 32-bit color value
										 *	 \param[in] c 32-bit RGBA color value
										 *	 \return Blue component of value
										 */

#define ALPHA(c)	((c)		&0xFF)	/*!< Gets the alpha component intensity from a 32-bit color value.
										 *	 \param[in] c 32-bit RGBA color value
										 *	 \return Alpha component of value
										 */

static color3* colors;
static const int COLOR_AMOUNT = 128;

void ColorManager_LoadColors()
{
    colors = (color3*)malloc(sizeof(color3) * COLOR_AMOUNT);

    colors[ColorWhite] = ColorManager_HexToColor(0xffffffff);
    colors[ColorBlack] = ColorManager_HexToColor(0x000000ff);
}

color3* ColorManager_Get(short index)
{
    if (index >=0 && index < COLOR_AMOUNT)
    {
        return &colors[index];
    }
    return &colors[ColorWhite];
}

color3* ColorManager_GetName(enum ColorName name)
{
    if (name >=0 && name < COLOR_AMOUNT)
    {
        return &colors[name];
    }
    return &colors[ColorWhite];
}

color3 ColorManager_HexToColor(unsigned int hex)
{
    color3 c = {0};
    c.r = RED(hex) / 255.0f;
    c.g = GREEN(hex) / 255.0f;
    c.b = BLUE(hex) / 255.0f;
    return c;
}

#undef RED
#undef GREEN
#undef BLUE
#undef ALPHA

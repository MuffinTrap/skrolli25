#ifndef PIXEL_FONT_H
#define PIXEL_FONT_H

/**
 * @brief Simple pixelfont
 */
struct PixelFont
{
    GLuint textureName;     /**< OpenGL texture name */
    short cw;               /**< Width of character in pixels */
    short ch;               /**< Height of character in pixels */
    short imageW;           /**< Width of the texture in pixels */
    short imageH;           /**< Height of the texture in pixels */
    char firstChar;         /**< First character of the font */
    char lastChar;          /**< Last character of the font */
};
typedef struct PixelFont PixelFont;

/**
 * @brief Loads a hard coded pixel font with ASCII set of characters
 * @note Each time this function is called, the font is loaded again and more memory gets used.
 * @return Pointer to the loaded font.
 */
PixelFont* PixelFont_LoadDebugFont(void);

/**
 * @brief Draws text on the screen using a PixelFont
 * @details Draws text on the screen. Reacts to line break and expands tab to 4 spaces. Letters are drawn in light grey. GlColor is set to full white after drawing.
 * @param font The PixelFont used for drawing
 * @param x X of top left corner of the first letter
 * @param y Y of top left corner of the first letter
 * @param scale Scale of letters. 1.0f is unmodified
 * @param buffer Buffer containing the text to draw.
 * @param maxLength How many letters to draw at maximum
 * @return How many lines were drawn
 */
short PixelFont_DrawText(PixelFont* font, short x, short y, float scale, const char* buffer, short maxLength);

#endif

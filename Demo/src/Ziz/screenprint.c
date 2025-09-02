#include "screenprint.h"
#include "pixel_font.h"

#include <ctoy.h>
#include <string.h>
#include <stdio.h>
#include <stdarg.h>

#include "opengl_include.h"

#define LINE_AMOUNT 120
#define LINE_LENGTH 80
static char * lineBuffer[LINE_AMOUNT];
static int showIndex = 0;
static int allocIndex = -1;
static float scale = 1.0f;

void screenprint_start_frame(void)
{
    showIndex = 0;
}

void screenprint_set_scale(float scaleParam)
{
    scale = scaleParam;

}

void screenprintf(const char* formatString, ... )
{
    static char screenprintBuffer[LINE_LENGTH];

    if (showIndex + 1 < LINE_AMOUNT)
    {
        va_list args;
        va_start(args, formatString);
        vsprintf(screenprintBuffer, formatString, args);
        va_end(args);

        if (showIndex > allocIndex)
        {
            lineBuffer[showIndex] = malloc(sizeof(char) * LINE_LENGTH);
            allocIndex = showIndex;
        }
        strncpy(lineBuffer[showIndex], screenprintBuffer, LINE_LENGTH);
        showIndex++;
    }
}

void screenprint_draw_prints(void)
{
    // Projection matrix
    glMatrixMode(GL_PROJECTION);
    glLoadIdentity();
    gluOrtho2D(0.0, (double)ctoy_frame_buffer_width(), 0.0, (double)ctoy_frame_buffer_height());
    glMatrixMode(GL_MODELVIEW);
    glLoadIdentity();
    glPushMatrix();
        glTranslatef(0.375f, 0.375f, 0.0f);

        static PixelFont* debugFont = NULL;
        if (debugFont == NULL)
        {
            debugFont = PixelFont_LoadDebugFont();
        }
        short dx = 0;
        short dy = ctoy_frame_buffer_height() - 32;
        for(int line = 0; line < showIndex; line++)
        {
            short lines = PixelFont_DrawText(debugFont, dx, dy, scale, lineBuffer[line], LINE_LENGTH);

            dx = 0;
            dy -= debugFont->ch * scale * lines;
            if (dy <= 0)
            {
                break;
            }
        }
        glScalef(1.0f, 1.0f, 1.0f);
    glPopMatrix();
}

void screenprint_free_memory(void)
{
    for(int line = 0; line < allocIndex; line++)
    {
        free(lineBuffer[line]);
    }
}

#undef LINE_LENGTH

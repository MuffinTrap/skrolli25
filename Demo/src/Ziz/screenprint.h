#ifndef SCREENPRINT_H
#define SCREENPRINT_H

/**
 * @file screenprint.h
 * @brief function for printing debug info on the window using default font.
 */


/**
 * @brief Call this at the start of the frame to reset layout position
 */
void screenprint_start_frame(void);


/**
 * @brief Draw text on the screen using the debug font
 * @details First line of text is drawn to the upper left corner of the screen. Subsequent calls are drawn under the previous ones. If the bottom of screen is reached this function will not try to draw.
 * @param formatString printf style format string.
 * @param va_args Parameters for format string, these must always be supplied.
 */
void screenprintf_impl(const char* formatString, ... );
void screenprint_impl(const char* string);


/**
 * @brief Call this after all other drawing is done to draw the messages printed.
 */
void screenprint_draw_prints_impl(void);

/**
 * @brief Releases all the memory in linebuffer
 */
void screenprint_free_memory(void);

/**
 * @brief Set the scale of text
 */
void screenprint_set_scale(float scale);

#   ifdef ZIZ_DISABLE_SCREENPRINT
#   define screenprintf(format, ...)
#   define screenprintf(string)
#   define screenprint_draw_prints()
#   define LINE_AMOUNT 1
#   define LINE_LENGTH 1
#   else
#   define LINE_AMOUNT 120
#   define LINE_LENGTH 80
#   ifdef GEKKO
#      define screenprintf(format, ...) screenprintf_impl(format, ##__VA_ARGS__)
#   else
#      define screenprintf(format, ...) screenprintf_impl(format, __VA_ARGS__)
#   endif
#   define screenprint(string) screenprint_impl(string)
#   define screenprint_draw_prints() screenprint_draw_prints_impl()

#   endif

#endif

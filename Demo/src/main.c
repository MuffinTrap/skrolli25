
// Headers
#include "../include/display.h"
#include <ctoy.h>
#include "Ziz/pixel_font.c"
#include "Ziz/screenprint.c"
#include "Ziz/opengl_include.h"

void ctoy_begin(void)
{
	ctoy_window_title("Bnuy");
	display_init(RESOLUTION_640x480, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);
}

void ctoy_end(void)
{
	screenprint_free_memory();
}

void clear_screen( void )
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
#ifdef GEKKO
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, ctoy_frame_buffer_width(), ctoy_frame_buffer_height());
#else
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
}

void start_frame_3D( void )
{
	glEnable(GL_DEPTH_TEST);
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

	gluLookAt(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);
}

void start_frame_2D( void )
{

    glDisable(GL_TEXTURE_2D);
	/*
    glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    */

	clear_screen();

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, (double)ctoy_frame_buffer_width(), 0.0, (double)ctoy_frame_buffer_height());
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);
}

void tri()
{
	glPushMatrix();

	glBegin(GL_TRIANGLES);
	glVertex2f(10.0f, 10.0f);
	glVertex2f(100.0f, 10.0f);
	glVertex2f(50.0f, 100.0f);
	glEnd();
	glPopMatrix();

}

void ctoy_main_loop(void)
{
	start_frame_2D();
	screenprint_start_frame();
	screenprint_set_scale(2.0f);

	glColor3f(1.0f, 1.0f, 1.0f);
	tri();
	glTranslatef(100.0f, 0.0f, 0.0f);
	tri();

	glTranslatef(-90.0f, 2.0f, 0.0f);
	glColor3f(0.8f, 0.2f, 0.3f);
	glScalef(0.8f, 0.8f, 1.0f);
	tri();
	glTranslatef(130.0f, 0.0f, 0.0f);
	tri();

	screenprintf("I am all ears");
	screenprint_draw_prints();

	ctoy_swap_buffer(NULL);
}

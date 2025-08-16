
// Headers
#define M_MATH_IMPLEMENTATION
#include <m_math.h>

#include "../include/display.h"
#include <ctoy.h>
#include "Ziz/opengl_include.h"
#include "Fx/koch_flake.h"


// Code
#include "Ziz/pixel_font.c"
#include "Ziz/screenprint.c"
#include "Fx/koch_flake.c"

static KochFlake flake;


void ctoy_begin(void)
{
	ctoy_window_title("Bnuy");
	display_init(RESOLUTION_640x480, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);


	flake.recursive_list = PointList_create(3);
	flake.local_list = PointList_create(3);
}

void ctoy_end(void)
{
	screenprint_free_memory();
}

void clear_screen( void )
{
	glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
	glViewport(0, 0, ctoy_frame_buffer_width(), ctoy_frame_buffer_height());
#ifdef GEKKO
	glClear(GL_COLOR_BUFFER_BIT);
#else
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
}

void start_frame_3D( void )
{
	//glEnable(GL_DEPTH_TEST);
    //glEnable(GL_CULL_FACE);
    //glCullFace(GL_BACK);
	glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 4.0/3.0, 0.01f, 1000.0f);
	gluLookAt(0.0f, 0.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void start_frame_2D( void )
{

    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_LIGHTING);
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluOrtho2D(0.0f, (double)ctoy_frame_buffer_width(), 0.0, (double)ctoy_frame_buffer_height());

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	glTranslatef(0.375f, 0.375f, 0.0f);
}

void tri()
{
	glBegin(GL_TRIANGLES);
		glVertex2f(10.0f, 10.0f);
		glVertex2f(100.0f, 10.0f);
		glVertex2f(50.0f, 100.0f);
	glEnd();
}

void ctoy_main_loop(void)
{
	clear_screen();
	start_frame_2D();
	screenprint_start_frame();
	screenprint_set_scale(2.0f);

	glPushMatrix();
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
	glPopMatrix();


	start_frame_3D();
	//float2 c = {ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2};
	float3 c3 = {0.0f, 0.0f, -280.0f};

	glPushMatrix();

	glTranslatef(c3.x, c3.y, c3.z);
	//glRotatef(45.0f, 1.0f, 0.0f, 1.0f);
	glRotatef(ctoy_get_time()*60.0f, 0.3f, 1.0f, 0.0f);
	//glRotatef((sin(ctoy_get_time()) + 2.0f) * 360, 1.0f, sin(ctoy_get_time()/2.0f), 1.0f);

	float2 pos = {0.0f, 0.0f};
	flake.center = pos;
	flake.radius = 160.0f;
	flake.recursion_level = 3;
	flake.angle = 60;
	flake.ratio = 1.0f/3.0f;
	flake.extrusion = 1.0f;

		draw_snowflake_struct(&flake);
		// void draw_snowflake(float2 center, float radius, short recursion, float angle, float ratio, float extrusion, PointList* recursive_list, PointList* local_list);
	glPopMatrix();

	screenprintf("I am all ears");
	screenprint_draw_prints();

	ctoy_swap_buffer(NULL);
}

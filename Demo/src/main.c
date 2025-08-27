#include <stdint.h>


// Headers
#define M_MATH_IMPLEMENTATION
#include <m_math.h>

#include <ctoy.h>
#include <display.h>
#include <opengl_include.h>
#include <texture.h>

#include "Ziz/screenprint.h"
#include "Ziz/ObjModel.h"

#include "Fx/pointlist.h"
#include "Fx/koch_flake.h"
#include "Fx/rotation_fx.h"
#include "Fx/flake_wheel_fx.h"
#include "Fx/bunny_fx.h"
#include "Fx/color_manager.h"
#include "Fx/gradient.h"
#include "Fx/gradient_mesh.h"
#include "Fx/gosper_curve.h"
//#include "../rocket/rocket_ctoy.h"


// Code
#ifdef GEKKO
	// Wii special things here
#else
// NOTE gcc compiles texture.c separately and includes stb_image.h twice
#include "../include/texture.c"
#include "../include/m_float2_math.c"

#include "Ziz/ObjModel.c"
#include "Ziz/pixel_font.c"
#include "Ziz/screenprint.c"
#include "Fx/pointlist.c"
#include "Fx/koch_flake.c"
#include "Fx/rotation_fx.c"
#include "Fx/flake_wheel_fx.c"
#include "Fx/bunny_fx.c"
#include "Fx/color_manager.c"
#include "Fx/gradient.c"
#include "Fx/gradient_mesh.c"
#include "Fx/gosper_curve.c"
#endif

// Rocket tracks
static int track_scene;	// Currently active scene
static int track_translate_x; // glTranslate x for effect
static int track_translate_y; // glTranslate y for effect
static int track_translate_z; // glTranslate z for effect
static int track_scale_xyz;	  // glScale x,y,z for effect


static KochFlake flake;
static PointList rotation_outer;

// Bunny fx
static struct Bunny bunny_mesh;

// Bunny gradient
static struct GradientMesh gradient_mesh;
static struct Gradient rainbow_gradient;

// Gosper curve fx
static PointList gosper_list;
static float gosper_length = 0.0f;
static float gosper_speed = 0.0f;

static void init_rocket_tracks(void)
{
	set_BPM(144.0f);
	set_RPB(4.0f);
	track_scene = add_to_rocket("scene");
	track_translate_x = add_to_rocket("translate_x");
	track_translate_y = add_to_rocket("translate_y");
	track_translate_z = add_to_rocket("translate_z");
	track_scale_xyz = add_to_rocket("scale_xyz");
}


void ctoy_begin(void)
{
	ctoy_window_title("Bnuy");
	display_init(RESOLUTION_640x480, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);


	flake.recursive_list = PointList_create(3);
	flake.local_list = PointList_create(3);
	rotation_outer = PointList_create(6);

	bunny_mesh = Bunny_Load("assets/bunny_medium.glb");

	ColorManager_LoadColors();
	rainbow_gradient = Gradient_CreateEmpty();
	Gradient_PushColor(&rainbow_gradient, ColorManager_GetName(ColorRose), 0.0f);
	Gradient_PushColor(&rainbow_gradient, ColorManager_GetName(ColorPurple), 0.5f);
	Gradient_PushColor(&rainbow_gradient, ColorManager_GetName(ColorCyanblue), 1.0f);

	int bunny_texture_id = addTexture("assets/lost_bun.png");
    GLuint gl_tex_name = bind_texture(bunny_texture_id);

	gradient_mesh = GradientMesh_Create(&rainbow_gradient, gl_tex_name, GradientVertical);

	float2 gstart = {00.0f, 00.0f};
	float2 gdir = {0.0f, 1.0f};
	float gosper_lenght = 5.0f;
	short gosper_recursion = 2;
	gosper_list = Gosper_Create(gstart, gdir, gosper_lenght, gosper_recursion);

	init_rocket_tracks();
}

void ctoy_end(void)
{
	screenprint_free_memory();
}

void clear_screen( void )
{
	glClearColor(0.3f, 0.2f, 0.4f, 1.0f);
	glViewport(0, 0, ctoy_frame_buffer_width(), ctoy_frame_buffer_height());
#ifdef GEKKO
	glClear(GL_COLOR_BUFFER_BIT);
#else
	glClear(GL_COLOR_BUFFER_BIT|GL_DEPTH_BUFFER_BIT);
#endif
}

void start_frame_3D( void )
{
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 4.0/3.0, 0.01f, 1000.0f);
	gluLookAt(0.0f, 0.0f, 1.0f,
			  0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
}

void start_frame_2D( void )
{

    //glDisable(GL_TEXTURE_2D);
    //glDisable(GL_LIGHTING);
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

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

void fx_ears()
{
	// Ears
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

}

void fx_gradient_bunny()
{
	short x = get_from_rocket(track_translate_x);
	short y = get_from_rocket(track_translate_y);
	float scale = get_from_rocket(track_scale_xyz);
	start_frame_2D();
	glPushMatrix();

		glTranslatef(ctoy_frame_buffer_width()/2+x, ctoy_frame_buffer_height()/2+y, 0.0f);
		glScalef(scale, scale, 1.0f);

		GradientMesh_Draw(&gradient_mesh);
	glPopMatrix();

}

void fx_gosper_curve()
{
	short x = get_from_rocket(track_translate_x);
	short y = get_from_rocket(track_translate_y);
	float scale = get_from_rocket(track_scale_xyz);
	start_frame_2D();
	glPushMatrix();

		glTranslatef(ctoy_frame_buffer_width()/2+x, ctoy_frame_buffer_height()/2+y, 0.0f);
		glScalef(scale, scale, 1.0f);
		glColor3f(1.0f, 1.0f, 1.0f);

		Gosper_Draw(&gosper_list, gosper_length);
	glPopMatrix();

	gosper_speed = 0.1f;
	gosper_length += gosper_speed;
}

void fx_stanford_bunny()
{
	start_frame_3D();
	glPushMatrix();

	float3 c3 = {0.0f, 0.0f, 0.7f};
	glTranslatef(c3.x, c3.y, c3.z);

	glRotatef(ctoy_get_time()*60.0f, 0.3f, 1.0f, 0.0f);
	glColor3f(1.0f, 1.0f, 1.0f);
	Bunny_Draw_immediate(&bunny_mesh, DrawTriangles);

	glPopMatrix();

}

void fx_rotation_illusion()
{
	float2 center = {ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2};
	glPushMatrix();
	//float3 color1 = {0.8f, 0.2f, 0.35f};
	float3 color1 = {0.2f, 0.2f, 0.2f};
	float3 color2 = {0.2f, 0.2f, 0.5f};
	float speed = 4.0f;
	float scale = 3.0f;
	float progress = ctoy_get_time()/5;
	rotation_fx(center, scale, speed, progress, color1, color2,
				&rotation_outer, &flake.recursive_list, &flake.local_list);
	glPopMatrix();
}

void fx_single_flake()
{

	float3 c3 = {0.0f, 0.0f, -280.0f};
	glTranslatef(c3.x, c3.y, c3.z);
	glRotatef(ctoy_get_time()*60.0f, 0.3f, 1.0f, 0.0f);

	float2 center = {ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2};
	glPushMatrix();
	float2 pos = center;
	flake.center = pos;
	flake.radius = 160.0f;
	flake.recursion_level = 3;
	flake.angle = 60;
	flake.ratio = 1.0f/3.0f;
	flake.extrusion = 1.0f;

	draw_snowflake_struct(&flake);
	glPopMatrix();
}

void fx_flake_wheel()
{

	glPushMatrix();

	float2 center = {ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2};
	//glTranslatef(center.x/8, center.y/8, 0.0f);
	//glRotatef(ctoy_get_time()*60.0f, 0.0f, 0.0f, 1.0f);
	float time = ctoy_get_time();
	flake_wheel_fx(center,
				   420.0f + sin(time*2)*120.0f,
				   240.0f + sin(time*4)*100.0f,
				   sin(ctoy_get_time()/20) * 80, sin(ctoy_get_time()/50) * -50, ctoy_get_time() *40, &rotation_outer, &flake.recursive_list, &flake.local_list );
	glPopMatrix();
}


void ctoy_main_loop(void)
{
	clear_screen();
	start_frame_2D();
	screenprint_start_frame();
	screenprint_set_scale(2.0f);

	float scene_number = get_from_rocket(track_scene);
	screenprintf("Active scene %.0f", scene_number);
	const int scene = scene_number;
	switch(scene)
	{
		case 0:
			fx_gradient_bunny();
			break;
		case 1:
			fx_gosper_curve();
			break;
		case 2:
			fx_single_flake();
			break;
		case 3:
			fx_flake_wheel();
			break;
		case 4:
			fx_stanford_bunny();
			break;
	}


	screenprintf("I am all ears");
	screenprint_draw_prints();

	ctoy_swap_buffer(NULL);
}

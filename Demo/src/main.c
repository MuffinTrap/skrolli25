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
#include "Fx/gradient_texture.h"
#include "Fx/gosper_curve.h"


// Code
#ifdef GEKKO
	// Wii special things here
#else
// NOTE gcc compiles texture.c separately and includes stb_image.h twice
#include "../include/texture.c"
#include "../include/m_float2_math.c"

#include "Ziz/mesh.c"
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
#include "Fx/gradient_texture.c"
#include "Fx/gosper_curve.c"
#endif

// screen dimensions
static float center_x;
static float center_y;

// Rocket tracks
// Timing
static int track_row;
static float row_rate;
static float bpm = 144.0f;
static float rpb = 4.0f;

// Common
static int track_scene;	// Currently active scene
static int track_translate_x; // glTranslate x for effect
static int track_translate_y; // glTranslate y for effect
static int track_translate_z; // glTranslate z for effect
static int track_scale_xyz;	  // glScale x,y,z for effect
static int track_rotation_x;	  // glRotate x,y,z for effect
static int track_rotation_y;	  // glRotate x,y,z for effect
static int track_rotation_z;	  // glRotate x,y,z for effect

// Fx tracks

// Gradient
static int track_gradient_mode;
static int track_gradient_shape;
static int track_gradient_offset;
static int track_gradient_repeat;
static int track_gradient_index;
static int track_gradient_size;

// 2D bunny and logo and credits
static int track_bunny_index;
static int track_bunny_x;
static int track_bunny_y;
static int track_bunny_size;

// Flake drawing offsets
static int track_flake_ratio_off;
static int track_flake_angle_off;
static int track_flake_extrusion_off;

// Flake tunnel tracks
static int track_tunnel_shapes;
static int track_tunnel_scale_step;
static int track_tunnel_rotation_step;
static int track_tunnel_gradient_step;

// Gosper curve
static int track_gosper_segments;
static int track_gosper_follow_on;

// Stanford bunny
static int track_stanford_light_on;
static int track_stanford_light_dir;

// TODO Scissors rectangles


// Variables


// Koch flakes
static KochFlake flake;
static PointList rotation_outer;
static PointList wheel_list;
static struct Mesh flake_mesh_recursion3;
static struct Mesh flake_mesh_recursion4;

// Bunny fx
static struct Bunny bunny_mesh;

// Bunny gradient
static struct GradientTexture lost_bunny_texture;
static struct GradientTexture falling_bunny_texture;
static struct GradientTexture logo_texture;

// Gradients
static struct Gradient white_gradient;
static struct Gradient rainbow_gradient;
static struct Gradient halo_gradient;

// Gosper curve fx
static PointList gosper_list;


static void init_rocket_tracks(void)
{
	set_BPM(bpm);
	set_RPB(rpb);
    row_rate = (bpm / 60.0) * rpb;
	track_row = add_to_rocket("row");
	track_scene = add_to_rocket("scene");
	track_translate_x = add_to_rocket("translate_x");
	track_translate_y = add_to_rocket("translate_y");
	track_translate_z = add_to_rocket("translate_z");
	track_scale_xyz = add_to_rocket("scale_xyz");
	track_rotation_x = add_to_rocket("rotation_x");
	track_rotation_y = add_to_rocket("rotation_y");
	track_rotation_z = add_to_rocket("rotation_z");

	track_gradient_mode = add_to_rocket("gradient_mode");
	track_gradient_shape = add_to_rocket("gradient_shape");
	track_gradient_offset = add_to_rocket("gradient_offset");
	track_gradient_repeat = add_to_rocket("gradient_repeat");
	track_gradient_index = add_to_rocket("gradient_index");
	track_gradient_size = add_to_rocket("gradient_size");

	track_bunny_index = add_to_rocket("bunny_index");
	track_bunny_x = add_to_rocket("bunny_x");
	track_bunny_y = add_to_rocket("bunny_y");
	track_bunny_size = add_to_rocket("bunny_size");

	track_flake_ratio_off = add_to_rocket("flake_ratio_off");
	track_flake_angle_off = add_to_rocket("flake_angle_off");
	track_flake_extrusion_off = add_to_rocket("flake_extrusion_off");

	// Flake tunnel tracks
	track_tunnel_shapes = add_to_rocket("tunnel_shapes");
	track_tunnel_scale_step = add_to_rocket("tunnel_scale_step");
	track_tunnel_rotation_step = add_to_rocket("tunnel_rotation_step");
	track_tunnel_gradient_step = add_to_rocket("tunnel_gradient_step");

	// Gosper curve tracks
	track_gosper_follow_on = add_to_rocket("gosper_follow");
	track_gosper_segments = add_to_rocket("gosper_segments");

	track_stanford_light_on = add_to_rocket("stfrd_light_on");
	track_stanford_light_dir = add_to_rocket("stfrd_light_dir");
}


void ctoy_begin(void)
{
	ctoy_window_title("Bnuy");
	display_init(RESOLUTION_640x480, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);

	// Create flake meshes
	rotation_outer = PointList_create(6);
	wheel_list = PointList_create(6);
	flake = KochFlake_CreateDefault(4);
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
	flake.recursion_level = 3;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion3);

	// Stanford bunny
	bunny_mesh = Bunny_Load("assets/bunny_medium.glb");

	// Colors and gradients
	ColorManager_LoadColors();
	rainbow_gradient = Gradient_CreateEmpty(GradientCircle, GradientLoopRepeat);
	{
		enum ColorName rainbow[] = {
			ColorRose,
			ColorDarkOrange,
			ColorOrange,
			ColorLightOrange,
			ColorOliveGreen,
			ColorGreen,
			ColorCyanBlue,
			ColorBlue,
			ColorPurple
		};
		Gradient_PushColorArray(&rainbow_gradient, rainbow, 9);
	}

	white_gradient = Gradient_CreateEmpty(GradientVertical, GradientLoopRepeat);
	{
		Gradient_PushColor(&white_gradient, ColorManager_GetName(ColorWhite), 0.0f);
		Gradient_PushColor(&white_gradient, ColorManager_GetName(ColorWhite), 1.0f);
	}

	halo_gradient = Gradient_CreateEmpty(GradientCircle, GradientLoopRepeat);
	{
		Gradient_PushName(&halo_gradient, ColorBlackBlue, 0.0f);
		Gradient_PushName(&halo_gradient, ColorCyanBlue, 0.4f);
		Gradient_PushName(&halo_gradient, ColorWhite, 0.5f);
		Gradient_PushName(&halo_gradient, ColorCyanBlue, 0.6f);
		Gradient_PushName(&halo_gradient, ColorBlackBlue, 1.0f);

	}

	// Bunny pictures
	int lost_bunny_texture_id = addTexture("assets/lost_bun.png");
	int falling_bunny_texture_id = addTexture("assets/falling_bun.png");

	int logo_texture_id = addTexture("assets/logo.png");

	lost_bunny_texture = GradientTexture_Create(
		bind_texture(lost_bunny_texture_id),
		lost_bunny_texture_id,
		GradientMultiply);

	falling_bunny_texture = GradientTexture_Create(
		bind_texture(falling_bunny_texture_id),
		falling_bunny_texture_id,
		GradientMultiply);

	logo_texture = GradientTexture_Create(
		bind_texture(logo_texture_id),
		logo_texture_id,
		GradientCutout
	);

	// Gosper curve
	float2 gstart = {00.0f, 00.0f};
	float2 gdir = {0.0f, 1.0f};
	float gosper_lenght = 5.0f;
	short gosper_recursion = 3;
	float gosper_width = 2.0f;
	gosper_list = Gosper_Create(gstart, gdir, gosper_lenght, gosper_width, gosper_recursion);

	init_rocket_tracks();

}

void ctoy_end(void)
{
	screenprint_free_memory();
}

void clear_screen( void )
{
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
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
    // glEnable(GL_BLEND);
    // glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glDisable(GL_DEPTH_TEST);

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

struct Gradient* select_gradient()
{
	struct Gradient* grad;
	switch( (int)get_from_rocket(track_gradient_index))
	{
		case 0: grad = &rainbow_gradient; break;
		case 1: grad = &halo_gradient; break;
		default: grad = &white_gradient; break;
	}

	int shape = (int)get_from_rocket(track_gradient_shape);
	screenprintf("Selected shape %d", shape);

	float repeat = get_from_rocket(track_gradient_repeat);

	switch(shape)
	{
		case GradientVertical:
			grad->shape = GradientVertical;
			break;
		case GradientRadial:
			grad->shape = GradientRadial;
			break;
		case GradientCircle:
			grad->shape = GradientCircle;
			break;
		default:
			grad->shape = GradientVertical;
	}
	return grad;
}

struct GradientTexture* select_texture()
{
	struct GradientTexture* text;
	switch((int)get_from_rocket(track_bunny_index))
	{
		case 0: text = &lost_bunny_texture; break;
		case 1: text = &falling_bunny_texture; break;
		case 2: text = &logo_texture; break;
		default: return NULL;
	}
	switch((int)get_from_rocket(track_gradient_mode))
	{
		case GradientMultiply:
			text->alphamode = GradientMultiply;
			break;
		case GradientCutout:
			text->alphamode = GradientCutout;
			break;
	}
	return text;
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
	float offset = get_from_rocket(track_gradient_offset);
	float repeat = get_from_rocket(track_gradient_repeat);
	float gradient_size = get_from_rocket(track_gradient_size);
	struct Gradient* grad = select_gradient();

	struct GradientTexture* text = select_texture();



	start_frame_2D();
	glPushMatrix();

		glTranslatef(ctoy_frame_buffer_width()/2+x, ctoy_frame_buffer_height()/2+y, 0.0f);
		glScalef(1.0f, 1.0f, 1.0f);

		GradientTexture_Draw(&lost_bunny_texture, &rainbow_gradient, scale, gradient_size, offset, repeat);
	glPopMatrix();

}

void fx_gosper_curve()
{
	static float2 last_point;
	short x = get_from_rocket(track_translate_x);
	short y = get_from_rocket(track_translate_y);
	float rotz = get_from_rocket(track_rotation_z);
	float scale = get_from_rocket(track_scale_xyz);
	start_frame_2D();
	glPushMatrix();

		if (get_from_rocket(track_gosper_follow_on) > 0.0f)
		{
			x = -last_point.x;
			y = -last_point.y;
		}
		glTranslatef(ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2, 0.0f);
		glRotatef(rotz, 0.0f, 0.0f, 1.0f);
		glScalef(scale, scale, 1.0f);
		glPushMatrix();

			// TODO Smooth follow of target
			glTranslatef(x, y, 0.0f);
			last_point = Gosper_Draw(&gosper_list, &rainbow_gradient, get_from_rocket(track_gosper_segments));
		glPopMatrix();
	glPopMatrix();
}

void fx_stanford_bunny()
{
	// Draw gradient bg
	struct Gradient* bg_grad = select_gradient();
	if (bg_grad != &white_gradient)
	{
		screenprintf("Gradient bg shape %d", bg_grad->shape);
		start_frame_2D();

		glPushMatrix();

			glTranslatef(ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2, 0.0f);
			glScalef(1.0f, 1.0f, 1.0f);
			glBegin(GL_LINES);
			glColor3f(1.0f, 1.0f, 1.0f);
			glVertex2f(0.0f, 0.0f);
			glVertex2f(100.0f, 0.0f);

			glEnd();

			GradientTexture_DrawGradient(bg_grad, GradientCutout,
										 get_from_rocket(track_gradient_size), get_from_rocket(track_gradient_offset), get_from_rocket(track_gradient_repeat));
		glPopMatrix();
	}

	start_frame_3D();
	bool lights_on = get_from_rocket(track_stanford_light_on) > 0.0f;
	static float3 y_axis = {0.0f, 1.0f, 0.0f};

	if (lights_on)
	{
		float matrix[] = M_MAT4_IDENTITY();
		m_mat4_rotation_axis(matrix, &y_axis, get_from_rocket(track_stanford_light_dir) * M_DEG_TO_RAD);
		float3 dir = {0.0f, 0.0f, 1.0f};
		float3 light_dir;
		m_mat4_transform3(
			&light_dir,
			matrix,
			&dir);
		// w == 0 makes this a directional light
		GLfloat light_dir_4[] = {light_dir.x, light_dir.y, light_dir.z, 0.0f};
		GLfloat mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
		GLfloat mat_shiny[] = {50.0};
		GLfloat light_color[] = {1.0f, 1.0f, 1.0f, 1.0f};

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shiny);
		glLightfv(GL_LIGHT0, GL_POSITION, light_dir_4);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);

		// Preserver normals when scaling
		glEnable(GL_NORMALIZE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);
	}
	glPushMatrix();

	glTranslatef(
		get_from_rocket(track_translate_x),
		get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);
	glRotatef(get_from_rocket(track_rotation_x), 1.0f, 0.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_y), 0.0f, 1.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_z), 0.0f, 0.0f, 1.0f);

	float scale = get_from_rocket(track_scale_xyz);
	glScalef(scale, scale, scale);
	glColor3f(1.0f, 1.0f, 1.0f);
	Bunny_Draw_immediate(&bunny_mesh, DrawTriangles);

	glPopMatrix();
	if (lights_on)
	{
		glDisable(GL_LIGHTING);
	}
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
				&rotation_outer, &flake.recursive_list);
	glPopMatrix();
}

void fx_flake_tunnel()
{
	start_frame_3D();

	glPushMatrix();
	flake.ratio = 1.0f/3.0f + get_from_rocket(track_flake_ratio_off) / 100.0f;
	flake.extrusion = 1.0f + get_from_rocket(track_flake_extrusion_off) / 100.0f;
	flake.angle = 60.0f + get_from_rocket(track_flake_angle_off);
	flake.recursion_level = 4;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);

	glDisable(GL_DEPTH_TEST);
	glTranslatef(
		get_from_rocket(track_translate_x),
		get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);
	glRotatef(get_from_rocket(track_rotation_x), 1.0f, 0.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_y), 0.0f, 1.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_z), 0.0f, 0.0f, 1.0f);

	float scale = get_from_rocket(track_scale_xyz);
	glScalef(scale, scale, 1.0f);

	float2 center = {0.0f, 0.0f};

	int shapes = (int)get_from_rocket(track_tunnel_shapes);
	float gradient_step = get_from_rocket(track_tunnel_gradient_step);

	float scale_step = get_from_rocket(track_tunnel_scale_step);
	float rotation_step = get_from_rocket(track_tunnel_rotation_step);
	screenprintf("Tunnel shapes %d", shapes);
	struct Gradient* grad = select_gradient();
	for(int f = 0; f < shapes; f++)
	{
		Gradient_glColor(grad, 0.00f + gradient_step * f);
		//glScalef(scale-1.0f*f, scale-1.0f*f, 1.0f);
		/*
		float2 pos = center;
		flake.center = pos;
		flake.radius = 1.0f;
		flake.recursion_level = 5;
		flake.angle = 60;
		flake.ratio = 1.0f/3.0f;
		flake.extrusion = 1.0f;

		draw_snowflake_struct(&flake);
		*/
		Mesh_DrawArrays(&flake_mesh_recursion4);
		glScalef(scale_step, scale_step, 1.0f);
		glRotatef(rotation_step, 0.0f, 0.0f, 1.0f);
	}

	glPopMatrix();

	// Overdraw bunny?
	struct GradientTexture* bunny = select_texture();
	if (bunny != NULL)
	{
		start_frame_2D();
		glPushMatrix();
		glTranslatef(
			center_x + get_from_rocket(track_bunny_x),
			center_y + get_from_rocket(track_bunny_y),
			0.0f
		);

		GradientTexture_DrawTexture(bunny, get_from_rocket(track_bunny_size));
		glPopMatrix();
	}

	glEnable(GL_DEPTH_TEST);
}

void fx_flake_wheel()
{
	start_frame_2D();
	glPushMatrix();

	glTranslatef(
		center_x + get_from_rocket(track_translate_x),
		center_y + get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);
	float2 center = {0.0f, 0.0f};
	float time = ctoy_get_time();
	flake_wheel_fx(center,
				   420.0f + sin(time*2)*120.0f,
				   240.0f + sin(time*4)*100.0f,
				   sin(ctoy_get_time()/20) * 80, sin(ctoy_get_time()/50) * -50, ctoy_get_time() *40,
				   select_gradient(),
				get_from_rocket(track_gradient_offset),
				   &rotation_outer, &flake.recursive_list, &wheel_list );
	glPopMatrix();
}

void update_timing()
{
	// TODO show scene duration
	float row = get_from_rocket(track_row);
	float seconds = row /row_rate;
	float minutes = seconds/60.0f;
	screenprintf("Music time %.0f:%.2f", minutes, (minutes-floor(minutes)) * 60.0f);
	float beat = row / rpb;
	float measures = beat/4.0f;
	short sbeat = floor(beat);
	screenprintf("Music bar/beat %.1f:%d", measures, sbeat%4 + 1) ;
}


void ctoy_main_loop(void)
{
	center_x = ctoy_frame_buffer_width()/2;
	center_y = ctoy_frame_buffer_height()/2;
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
			fx_flake_tunnel();
			break;
		case 3:
			fx_flake_wheel();
			break;
		case 4:
			fx_stanford_bunny();
			break;
		case 5:
			fx_rotation_illusion();
			break;
	}


	screenprintf("I am all ears");
	update_timing();
	screenprint_draw_prints();

	ctoy_swap_buffer(NULL);
}

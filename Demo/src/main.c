#include <stdint.h>
#include <limits.h>

//#define USE_RAT_MODELS

// Headers
#define M_MATH_IMPLEMENTATION
#include <m_math.h>

#include <ctoy.h>
#include <display.h>
#include <opengl_include.h>
#include <types.h>
#include <animation.h>
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
#   include "ufbx/ufbx.h"
#   include "ufbx/ufbx.c"
#	include "Fx/ufbx_to_mesh.c"

#endif

// NOTE gcc compiles texture.c separately and includes stb_image.h twice
#include <texture.c>
#include <m_float2_math.c>
#include <wii_memory_functions.c>
#include <frustum_culling.c>
//#include <rat_handler.c>
//#include <rat_actors.c>
//#include "../include/glb2rat.c"

/*
#ifndef UINT64_C
#define UINT64_C(c) c ## ULL
#endif
#ifndef SIZE_MAX
#define SIZE_MAX 4
#endif

*/

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

// screen dimensions
static float center_x;
static float center_y;

// Rocket tracks
// Timing
static int track_row;
static float row_rate;
static float bpm = 144.0f;
static float rpb = 4.0f;

#define FAR_PLANE 1000.0f
#define NEAR_PLANE 0.01f


// Koch flakes
static KochFlake flake;
static PointList rotation_outer;
static PointList wheel_list;
static struct Mesh flake_mesh_recursion3;
static struct Mesh flake_mesh_recursion4;

// Bunny fx
static struct Bunny bunny_mesh;
static struct Bunny test_mesh;

// Bunny gradient
static struct GradientTexture lost_bunny_texture;
static struct GradientTexture falling_bunny_texture;
static struct GradientTexture lynn_test_gs;
static struct GradientTexture lynn_test_rgb;
static struct GradientTexture logo_texture;
static struct GradientTexture matcap_green_orange;

// Gradients
static struct Gradient white_gradient;
static struct Gradient rainbow_gradient;
static struct Gradient halo_gradient;

// Gosper curve fx
static PointList gosper_list;

#include "main_rocket.h"

struct GradientTexture LoadImage(const char* filename)
{
	int texture_id = addTexture(filename);
	struct GradientTexture text;
	text = GradientTexture_Create(
		bind_texture(texture_id),
		texture_id,
		GradientMultiply);
	return text;
}

void ctoy_begin(void)
{
	ctoy_window_title("Bnuy");
	display_init(RESOLUTION_640x480, DEPTH_32_BPP, 2, GAMMA_NONE, FILTERS_DISABLED);

	// Bunny pictures
	lost_bunny_texture = LoadImage("assets/lost_bun.png");
	falling_bunny_texture = LoadImage("assets/falling_bun.png");
	logo_texture = LoadImage("assets/logo.png");
	matcap_green_orange = LoadImage("assets/matcap.png");

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
		Gradient_PushName(&halo_gradient, ColorBlue, 0.40f);
		Gradient_PushName(&halo_gradient, ColorCyanBlue, 0.49f);
		Gradient_PushName(&halo_gradient, ColorWhite, 0.5f);
		Gradient_PushName(&halo_gradient, ColorCyanBlue, 0.51);
		Gradient_PushName(&halo_gradient, ColorBlue, 0.60f);
		Gradient_PushName(&halo_gradient, ColorBlackBlue, 1.0f);

	}

	// Create flake meshes
	rotation_outer = PointList_create(6);
	wheel_list = PointList_create(6);
	flake = KochFlake_CreateDefault(4);
	flake_mesh_recursion3 = Mesh_CreateEmpty();
	flake_mesh_recursion4 = Mesh_CreateEmpty();
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
	flake.recursion_level = 3;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion3);

	// Stanford bunny
	//bunny_mesh = Bunny_Load_RAT("assets/bunny_medium.glb");
#	ifdef GEKKO
	bunny_mesh = Bunny_Load_UFBX("assets/bunny_medium.fbx");
#	else
	bunny_mesh = Bunny_Load_GLTF("assets/bunny_medium.glb");
#	endif
	Bunny_Allocate_Texcoords(&bunny_mesh);
	Mesh_PrintInfo(&bunny_mesh.mesh, false);

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
    glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE); //  is this needed?
	glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);


	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(90.0, 4.0/3.0, NEAR_PLANE, FAR_PLANE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0f, 0.0f, 1.0f,
			  0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f);
}

void start_frame_ortho_3D( void )
{
	glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LEQUAL);
	glDepthMask(GL_TRUE); //  is this needed?
	glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);

	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	float aspect = 4.0f/3.0f;
	glOrtho(-aspect, aspect, -1.0f, 1.0f, NEAR_PLANE, FAR_PLANE);

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();
	gluLookAt(0.0f, 0.0f, 1.0f,
			  0.0f, 0.0f, 0.0f,
			0.0f, 1.0f, 0.0f);

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
	grad->repeats = repeat;

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
struct GradientTexture* select_matcap()
{
	struct GradientTexture* text;
	int texture_index = (int)get_from_rocket(track_matcap_index);
	screenprintf("Selected matcap %d", texture_index);
	switch(texture_index)
	{
		case 0: text = &matcap_green_orange; break;
		default: text = &matcap_green_orange; break;
	}
	return text;
}

struct GradientTexture* select_texture()
{
	struct GradientTexture* text;
	int texture_index = (int)get_from_rocket(track_bunny_index);
	screenprintf("Selected texture %d", texture_index);
	switch(texture_index)
	{
		case 0: text = &lost_bunny_texture; break;
		case 1: text = &falling_bunny_texture; break;
		case 2: text = &logo_texture; break;
		case 3: text = &lynn_test_gs; break;
		case 4: text = &lynn_test_rgb; break;
		case 5: text = &matcap_green_orange; break;
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

void morph_flake(struct KochFlake* flake)
{
	flake->ratio = 1.0f/3.0f + get_from_rocket(track_flake_ratio_off) / 100.0f;
	flake->extrusion = 1.0f + get_from_rocket(track_flake_extrusion_off) / 100.0f;
	flake->angle = 60.0f + get_from_rocket(track_flake_angle_off);
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
	float gradient_size = get_from_rocket(track_gradient_size);
	struct Gradient* grad = select_gradient();

	struct GradientTexture* text = select_texture();
	start_frame_2D();
	glPushMatrix();

		glTranslatef(center_x+x, center_x+y, 0.0f);
		glScalef(1.0f, 1.0f, 1.0f);

		GradientTexture_Draw(text, grad, scale, gradient_size, offset);
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

static void draw_stanford(enum MeshDrawMode drawmode)
{
	//start_frame_3D();
	glTranslatef(
		get_from_rocket(track_translate_x),
		get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);
	glRotatef(get_from_rocket(track_rotation_x), 1.0f, 0.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_y), 0.0f, 1.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_z), 0.0f, 0.0f, 1.0f);

	float scale = get_from_rocket(track_scale_xyz) * 0.990f;
	glScalef(scale, scale, scale);

	glColor3f(1.0f, 1.0f, 1.0f);
	Bunny_Draw_mesh(&bunny_mesh, drawmode);
}

static void fx_matcap_bunny()
{
	start_frame_3D();
	glPushMatrix();
	draw_stanford(DrawLines);
	glPopMatrix();
	glDepthFunc(GL_LESS);
	bool cut= get_from_rocket(track_scissor_1_cut) > 0.0f;
	if (cut)
	{
		glEnable(GL_SCISSOR_TEST);
		float l = get_from_rocket(track_scissor_1_left);
		float r = get_from_rocket(track_scissor_1_right);
		float t = get_from_rocket(track_scissor_1_top);
		float b = get_from_rocket(track_scissor_1_bottom);
		glScissor(l, b, r-l, t-b );
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
		struct GradientTexture* material = select_matcap();
		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, material->gl_texture_name);
			struct Mesh* stfrd = &bunny_mesh.mesh;

			Mesh_GenerateMatcapUVs(stfrd);


			Mesh_EnableAttribute(stfrd, AttributeTexcoord);
				Bunny_Draw_mesh(&bunny_mesh, DrawTriangles);
			Mesh_DisableAttribute(stfrd, AttributeTexcoord);
		glDisable(GL_TEXTURE_2D);

	glPopMatrix();

	if (cut)
	{
		glDisable(GL_SCISSOR_TEST);
	}

}


void fx_stanford_bunny()
{
	// Draw gradient bg
	struct Gradient* bg_grad = select_gradient();
	if (bg_grad != &white_gradient)
	{
		screenprintf("Gradient bg shape %d", bg_grad->shape);
		start_frame_ortho_3D();

		glPushMatrix();

		// Make sure gradient stays behind bunny
			glTranslatef(0.0f,
						 0.0f,
						 -FAR_PLANE + 10.0f);

			float grad_size = get_from_rocket(track_gradient_size);
			glScalef(1.0f/grad_size * 2, 1.0/grad_size * 2, 1.0f);

			GradientTexture_DrawGradient(bg_grad, GradientCutout,
										 grad_size,
					get_from_rocket(track_gradient_offset));

		glPopMatrix();
	}

	//start_frame_3D();
	bool lights_on = get_from_rocket(track_stanford_light_on) > 0.0f;
	static float3 light_axis = {1.0f, 0.0f, 0.0f};

	if (lights_on)
	{
		float matrix[] = M_MAT4_IDENTITY();
		m_mat4_rotation_axis(matrix, &light_axis, get_from_rocket(track_stanford_light_dir) * M_DEG_TO_RAD);
		float3 dir = {0.0f, 0.0f, 1.0f};
		float3 light_dir;
		m_mat4_transform3(
			&light_dir,
			matrix,
			&dir);
		// w == 0 makes this a directional light
		GLfloat light_dir_4[] = {light_dir.x, light_dir.y, light_dir.z, 0.0f};
		GLfloat mat_specular[] = {1.0f, 1.0f, 1.0f, 1.0f};
		GLfloat mat_shiny[] = {100.0};

		float intensity = get_from_rocket(track_stanford_light_intensity);
		color3 light_col = Gradient_GetColor(bg_grad, get_from_rocket(track_stanford_light_color));
		color3 light_ambient_col = Gradient_GetColor(bg_grad, get_from_rocket(track_stanford_light_ambient_color));

		GLfloat light_color[] = {light_col.r * intensity, light_col.g * intensity, light_col.b * intensity, 1.0f};
		GLfloat ambient_light_color[] = {light_ambient_col.r * intensity, light_ambient_col.g * intensity, light_ambient_col.b * intensity, 1.0f};

		glMaterialfv(GL_FRONT, GL_SPECULAR, mat_specular);
		glMaterialfv(GL_FRONT, GL_SHININESS, mat_shiny);
		glLightfv(GL_LIGHT0, GL_POSITION, light_dir_4);
		glLightfv(GL_LIGHT0, GL_DIFFUSE, light_color);
		glLightfv(GL_LIGHT0, GL_SPECULAR, light_color);
		glLightfv(GL_LIGHT0, GL_AMBIENT, ambient_light_color);

		// Preserver normals when scaling
		glEnable(GL_NORMALIZE);
		glEnable(GL_LIGHTING);
		glEnable(GL_LIGHT0);

	}

	draw_stanford(DrawTriangles);

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

	float color1 = 0.0f;
	float color2 = 1.0f;
	float speed = 4.0f;
	float scale = 3.0f;
	float progress = ctoy_get_time()/5;
	rotation_fx(&flake_mesh_recursion4,
				scale, speed, progress,
			 select_gradient(),
			 color1, color2);
	glPopMatrix();
}

void fx_flake_tunnel()
{
	start_frame_3D();

	morph_flake(&flake);
	flake.recursion_level = 4;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
	glDisable(GL_DEPTH_TEST);

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
		glPushMatrix();
			glRotatef(rotation_step, 0.0f, 0.0f, 1.0f);
			float flake_scale = scale - scale_step * (f+1);
			screenprintf("Flake scale %d: %.2f", f, flake_scale);
			glScalef(flake_scale, flake_scale, 1.0f);
			Mesh_DrawArrays(&flake_mesh_recursion4, DrawTriangles);
		glPopMatrix();
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
	morph_flake(&flake);
	flake.recursion_level = 4;
	float old_radius = flake.radius;
	float scale = get_from_rocket(track_scale_xyz);
	flake.radius = scale;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
	flake.radius = old_radius;
	start_frame_2D();
	glPushMatrix();

	glTranslatef(
		center_x + get_from_rocket(track_translate_x),
		center_y + get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);
	glRotatef( get_from_rocket(track_rotation_z), 0.0f, 0.0f, 1.0f );
	float2 center = {0.0f, 0.0f};

	struct Gradient* grad = select_gradient();
	flake_wheel_fx(&flake_mesh_recursion4,
				   get_from_rocket(track_flake_wheel_radius),
				   get_from_rocket(track_flake_wheel_outer_radius),
				   get_from_rocket(track_flake_wheel_shape_rotation),
				   get_from_rocket(track_flake_wheel_pattern_rotation),
				   get_from_rocket(track_flake_wheel_outer_pattern_rotation),
				   grad,
				get_from_rocket(track_gradient_offset),
				get_from_rocket(track_flake_wheel_ring_color),
				get_from_rocket(track_flake_wheel_shape_color)
	);
	glPopMatrix();
}

void update_timing()
{
	// TODO show scene duration
	float row = get_from_rocket(track_row);
	float seconds = row /row_rate;
	float minutes = seconds/60.0f;
	float full_minutes = floor(minutes);
	screenprintf("Music time %.0f:%.1f", full_minutes, seconds - full_minutes * 60.0f);
	float beat = row / rpb;
	float measures = (beat)/4.0f + 1;
	short sbeat = floor(beat);
	screenprintf("Music beat/bar %d/%.0f", sbeat%4 + 1, floor(measures));

	static int prev_scene = 0;
	static float prev_duration_s = 0.0f;
	static float prev_start_time = 0.0f;
	if (row == 0)
	{
		// reset
		prev_scene = 0;
		prev_start_time = 0;
	}
	int scene = (int)get_from_rocket(track_scene);
	if (scene != prev_scene)
	{
		prev_duration_s = seconds - prev_start_time;
		prev_start_time = seconds;
		prev_scene = scene;
	}
	screenprintf("Scene duration %.1f", seconds-prev_start_time);
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
	screenprintf("I am all ears");
	screenprintf("Active scene %.0f", scene_number);
	update_timing();
	screenprintf("----------------", scene_number);
	const int scene = scene_number;

	// Wii testing
	/*
	//fx_stanford_bunny();
	*/
	/*
	fx_ears();
	draw_stanford();
	screenprintf("Bunny mesh obj id %d", bunny_mesh.obj_id);
	if (bunny_mesh.obj_id >= 0)
	{
		Mesh_PrintInfo(&bunny_mesh.mesh);
		if (bunny_mesh.mesh.positions != NULL)
		{
			screenprintf("Bunny mesh loaded");
		}
	}
	else
	{
		screenprintf("Bunny mesh load failed: %d", bunny_mesh.error_code);
	}
	*/
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

		case 6:
			fx_matcap_bunny();
			break;


		case 99:
			// Black screen
			break;
		case 999:
			// Quit
			break;
	}

	//screenprint_draw_prints();

	ctoy_swap_buffer(NULL);
}

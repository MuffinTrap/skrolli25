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
static struct Mesh flake_mesh_recursion4;

// Bunny fx
static struct Bunny bunny_mesh;
static struct Bunny test_mesh;

// Bunny gradient
static struct GradientTexture* bunnies;
static const int BUNNY_AMOUNT = 8;

static const int MATCAP_AMOUNT = 8;
static struct GradientTexture* matcaps;

// Gradients
static struct Gradient white_gradient;
static struct Gradient rainbow_gradient;
static struct Gradient cold_halo_gradient;
static struct Gradient warm_halo_gradient;
static struct Gradient cold_to_warm_gradient;


// Gosper curve fx
static PointList gosper_list;

static float gosper_lenght = 5.0f;
static short gosper_recursion = 3;
static float gosper_width = 2.0f;

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
	bunnies = (struct GradientTexture*)malloc(sizeof(struct GradientTexture) * BUNNY_AMOUNT);
	bunnies[0] = LoadImage("assets/bun_wow_1.png");
	bunnies[1] = LoadImage("assets/bun_wow_2.png");
	bunnies[2] = LoadImage("assets/bun_wow_3.png");

	bunnies[3] = LoadImage("assets/bun_falling.png");
	bunnies[4] = LoadImage("assets/bun_standing.png");
	bunnies[5] = LoadImage("assets/bun_angel.png");
	bunnies[6] = LoadImage("assets/logo.png");
	bunnies[7] = LoadImage("assets/bun_zen.png");

	// MATCAPS
	matcaps = (struct GradientTexture*)malloc(sizeof(struct GradientTexture) * MATCAP_AMOUNT);
	matcaps[2] = LoadImage("assets/mat_3.png");
	matcaps[3] = LoadImage("assets/mat_glass.png");
	matcaps[4] = LoadImage("assets/mat_8.png");
	matcaps[6] = LoadImage("assets/mat_gold.png");
	matcaps[7] = LoadImage("assets/mat_azure.png");

	// 7 2 3 6

	GradientTexture_SetFiltering(&bunnies[6], GL_NEAREST);

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

	cold_halo_gradient = Gradient_CreateEmpty(GradientCircle, GradientLoopRepeat);
	{
		Gradient_PushName(&cold_halo_gradient, ColorBlackBlue, 0.0f);
		Gradient_PushName(&cold_halo_gradient, ColorBlue, 0.40f);
		Gradient_PushName(&cold_halo_gradient, ColorCyanBlue, 0.49f);
		Gradient_PushName(&cold_halo_gradient, ColorWhite, 0.5f);
		Gradient_PushName(&cold_halo_gradient, ColorCyanBlue, 0.51);
		Gradient_PushName(&cold_halo_gradient, ColorBlue, 0.60f);
		Gradient_PushName(&cold_halo_gradient, ColorBlackBlue, 1.0f);

	}

	warm_halo_gradient = Gradient_CreateEmpty(GradientCircle, GradientLoopRepeat);
	{
		Gradient_PushName(&warm_halo_gradient, ColorDarkOrange, 0.0f);
		Gradient_PushName(&warm_halo_gradient, ColorOrange, 0.40f);
		Gradient_PushName(&warm_halo_gradient, ColorLightOrange, 0.45f);
		Gradient_PushName(&warm_halo_gradient, ColorWhite, 0.5f);
		Gradient_PushName(&warm_halo_gradient, ColorLightOrange, 0.55);
		Gradient_PushName(&warm_halo_gradient, ColorOrange, 0.60f);
		Gradient_PushName(&warm_halo_gradient, ColorDarkOrange, 1.0f);
	}

	cold_to_warm_gradient = Gradient_CreateEmpty(GradientVertical, GradientLoopMirror);
	{
		enum ColorName cold2warm[] = {
			ColorRose,
			ColorDarkOrange,
			ColorOrange,
			ColorLightOrange,

			ColorGreen,
			ColorOliveGreen,
			ColorBlue,

			ColorCyanBlue
		};
		Gradient_PushColorArray(&cold_to_warm_gradient, cold2warm,8 );
	}


	// Create flake meshes
	rotation_outer = PointList_create(6);
	wheel_list = PointList_create(6);
	flake = KochFlake_CreateDefault(4);
	flake_mesh_recursion4 = Mesh_CreateEmpty();
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);

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
	gosper_list = PointList_create(1200);
	float2 gstart = {00.0f, 00.0f};
	float2 gdir = {0.0f, 1.0f};
	Gosper_Create(&gosper_list, gstart, gdir, gosper_lenght, gosper_width, gosper_recursion);

	init_rocket_tracks();

}

void ctoy_end(void)
{
	screenprint_free_memory();
}

// Selection functions

#include "main_util_functions.h"


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

void fx_eva_bunny()
{
	float2 left_bunny_pos = {get_from_rocket(track_bunny_x), get_from_rocket(track_bunny_y)};
	float2 right_bunny_pos = {left_bunny_pos.x * -1.0f,
		left_bunny_pos.y * -1.0f};

	float2 bunny_size;
	bunny_size.y = get_from_rocket(track_bunny_size);
	float offset = get_from_rocket(track_gradient_offset);
	float bunny_rot = get_from_rocket(track_rotation_z);
	float2 gradient_pos = {get_from_rocket(track_translate_x), get_from_rocket(track_translate_y)};
	float gradient_size = get_from_rocket(track_gradient_size);
	struct Gradient* grad = select_gradient();

	struct GradientTexture* text = select_texture();
	if (text == NULL || grad == NULL)
	{
		return;
	}
	bunny_size.x = text->aspect_ratio * bunny_size.y;
	start_frame_2D();
	glPushMatrix();

		// Draw halo gradient
		glTranslatef(center_x, center_y, 0.0f);
		glScalef(1.0f, 1.0f, 1.0f);

		GradientTexture_DrawGradient(grad, GradientCutout, gradient_size, offset);

		// Draw 2 bunnies overlaid with gradient
		grad->shape = GradientVertical;
		text->alphamode = GradientMultiply;

		glEnable(GL_TEXTURE_2D);
		glBindTexture(GL_TEXTURE_2D, text->gl_texture_name);
		glEnable(GL_BLEND);
		glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

		// Bunny one with gradient
		glPushMatrix();
			glTranslatef(left_bunny_pos.x, left_bunny_pos.y, 0.0f);
			GradientTexture_DrawVerticalGradient(grad, bunny_size, true, offset);
		glPopMatrix();

		// Bunny two with gradient
		glPushMatrix();
			glTranslatef(right_bunny_pos.x, right_bunny_pos.y, 0.0f);
			glRotatef(180.0f, 0.0f, 0.0f, 1.0f);
			GradientTexture_DrawVerticalGradient(grad, bunny_size, true, offset);
		glPopMatrix();

		glDisable(GL_BLEND);
		glBindTexture(GL_TEXTURE_2D, 0);
		glDisable(GL_TEXTURE_2D);

	glPopMatrix();

}

void fx_gradient_bunny()
{
	float2 bunny_pos = {get_from_rocket(track_bunny_x), get_from_rocket(track_bunny_y)};
	float bunny_size = get_from_rocket(track_bunny_size);
	float offset = get_from_rocket(track_gradient_offset);
	float bunny_rot = get_from_rocket(track_rotation_z);
	float2 gradient_pos = {get_from_rocket(track_translate_x), get_from_rocket(track_translate_y)};
	float gradient_size = get_from_rocket(track_gradient_size);
	struct Gradient* grad = select_gradient();

	struct GradientTexture* text = select_texture();
	if (text == NULL || grad == NULL)
	{
		return;
	}
	start_frame_2D();
	glPushMatrix();

		glTranslatef(center_x, center_y, 0.0f);
		glScalef(1.0f, 1.0f, 1.0f);

		GradientTexture_DrawBunny(text, grad,
								  bunny_pos, bunny_size, bunny_rot,
							gradient_pos, gradient_size,
							offset);
	glPopMatrix();

}

void fx_gosper_curve()
{
	// Regenerate
	float2 gstart = {00.0f, 00.0f};
	float2 gdir = {0.0f, 1.0f};
	gosper_width = get_from_rocket(track_gosper_width) + 0.01f;
	Gosper_Create(&gosper_list, gstart, gdir, gosper_lenght, gosper_width, gosper_recursion);
	static float2 last_point;
	short target_x = 0;
	short x = get_from_rocket(track_translate_x);
	short target_y = 0;
	short y = get_from_rocket(track_translate_y);
	float rotz = get_from_rocket(track_rotation_z);
	float scale = get_from_rocket(track_scale_xyz);
	start_frame_2D();
	glPushMatrix();

		float mix = get_from_rocket(track_gosper_follow_mix);
		{
			target_x = (1.0f - mix) * x - last_point.x * mix;
			target_y = (1.0f - mix) * y - last_point.y * mix;
		}
		glTranslatef(ctoy_frame_buffer_width()/2, ctoy_frame_buffer_height()/2, 0.0f);
		glRotatef(rotz, 0.0f, 0.0f, 1.0f);
		glScalef(scale, scale, 1.0f);
		glPushMatrix();

			// TODO Smooth follow of target
			glTranslatef(target_x, target_y, 0.0f);
			last_point = Gosper_Draw(&gosper_list, select_gradient(), get_from_rocket(track_gosper_segments),
									 get_from_rocket(track_gradient_offset));
		glPopMatrix();
	glPopMatrix();
}

static void EnableLights()
{
	bool lights_on = get_from_rocket(track_stanford_light_on) > 0.0f;
	if (lights_on)
	{

		struct Gradient* bg_grad = select_gradient();
		static float3 light_axis = {1.0f, 0.0f, 0.0f};
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
		float ambient_intensity = get_from_rocket(track_stanford_light_ambient_intensity);
		color3 light_col = Gradient_GetColor(bg_grad, get_from_rocket(track_stanford_light_color));
		color3 light_ambient_col = Gradient_GetColor(bg_grad, get_from_rocket(track_stanford_light_ambient_color));

		GLfloat light_color[] = {
			light_col.r * intensity,
			light_col.g * intensity,
			light_col.b * intensity, 1.0f};

		GLfloat ambient_light_color[] = {
			light_ambient_col.r * ambient_intensity,
			light_ambient_col.g * ambient_intensity,
			light_ambient_col.b * ambient_intensity, 1.0f};

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
}

static void DisableLights()
{
	bool lights_on = get_from_rocket(track_stanford_light_on) > 0.0f;
	if (lights_on)
	{
		glDisable(GL_LIGHTING);
	}
}

static void do_scissors(int first_track)
{
	float l = get_from_rocket(first_track);
	float r = get_from_rocket(first_track + 1);
	float t = get_from_rocket(first_track + 2);
	float b = get_from_rocket(first_track + 3);
	glScissor(l, b, r-l, t-b );
}

static void draw_stanford(enum MeshDrawMode drawmode)
{
	//start_frame_3D();
	glTranslatef(
		get_from_rocket(track_translate_x),
		get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);

	rotate_by_rocket();

	float scale = get_from_rocket(track_scale_xyz) * 0.99899f;
	glScalef(scale, scale, scale);

	glColor3f(1.0f, 1.0f, 1.0f);
	Bunny_Draw_mesh(&bunny_mesh, drawmode);
}

static void draw_matcap_bunny()
{
	glPushMatrix();
		glTranslatef(
			get_from_rocket(track_translate_x),
			get_from_rocket(track_translate_y),
			get_from_rocket(track_translate_z)+0.01
		);
		rotate_by_rocket();

		scale_by_rocket(true);
		struct Mesh* stfrd = &bunny_mesh.mesh;
		Mesh_GenerateMatcapUVs(stfrd);



		float alpha = get_from_rocket(track_matcap_alpha);
		if (alpha < 1.0f)
		{
			glEnable(GL_BLEND);
			glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
			glColor4f(1.0f, 1.0f, 1.0f, alpha);
		}
		else
		{
			glColor3f(1.0f, 1.0f, 1.0f);
		}

		bool cut= get_from_rocket(track_scissor_1_cut) > 0.0f;
		if (cut)
		{
			glEnable(GL_SCISSOR_TEST);
			do_scissors(track_scissor_1_left);
		}
			Mesh_EnableAttribute(stfrd, AttributeTexcoord);
			glEnable(GL_TEXTURE_2D);

			int mats = get_from_rocket(track_matcap_amount);
			if (mats >= 1)
			{
				struct GradientTexture* material = select_matcap(track_matcap_index);
				glBindTexture(GL_TEXTURE_2D, material->gl_texture_name);
				Bunny_Draw_mesh(&bunny_mesh, DrawTriangles);
			}
			if (mats >= 2)
			{
				do_scissors(track_scissor_2_left);
				struct GradientTexture* material = select_matcap(track_matcap_index2);
				glBindTexture(GL_TEXTURE_2D, material->gl_texture_name);
				Bunny_Draw_mesh(&bunny_mesh, DrawTriangles);
			}
			if (mats >= 3)
			{
				do_scissors(track_scissor_3_left);
				struct GradientTexture* material = select_matcap(track_matcap_index3);
				glBindTexture(GL_TEXTURE_2D, material->gl_texture_name);
				Bunny_Draw_mesh(&bunny_mesh, DrawTriangles);
			}

			// More matcaps

			glDisable(GL_TEXTURE_2D);
			Mesh_DisableAttribute(stfrd, AttributeTexcoord);

		if (alpha < 1.0f)
		{
			glDisable(GL_BLEND);
		}
		glPopMatrix(); // Rotation y

	glPopMatrix(); // Full matrix

	if (cut)
	{
		glDisable(GL_SCISSOR_TEST);
	}
}


static void fx_matcap_bunny()
{
	start_frame_3D();

	float base_on = get_from_rocket(track_matcap_base_on);
	if (base_on > 0.0f)
	{
		glPushMatrix();
		EnableLights();
			draw_stanford(DrawTriangles);
			DisableLights();
		glPopMatrix();
	}

	draw_matcap_bunny();
}


void fx_stanford_bunny()
{
	start_frame_ortho_3D();
	// Draw gradient bg
	struct Gradient* bg_grad = select_gradient();
	if (bg_grad != &white_gradient)
	{
		screenprintf("Gradient bg shape %d", bg_grad->shape);

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


	EnableLights();
	glPushMatrix();

	draw_stanford(DrawTriangles);

	glPopMatrix();
	DisableLights();
	draw_matcap_bunny();
}

void fx_rotation_illusion()
{
	start_frame_2D();
	glPushMatrix();
		translate_by_rocket(center_x, center_y);
		rotate_by_rocket();
		//float3 color1 = {0.8f, 0.2f, 0.35f};

		struct Gradient* gradient = select_gradient();
		float foreground_stop = get_from_rocket(track_gradient_offset);
		float background_stop = get_from_rocket(track_background_color);
		float scale = get_from_rocket(track_scale_xyz);
		float progress = get_from_rocket(track_rotation_illusion_progress);

		const color3 fore = Gradient_GetColor(gradient, foreground_stop);
		const color3 back = Gradient_GetColor(gradient, background_stop);
		rotation_fx(
			&flake_mesh_recursion4,
				scale, progress,
			  fore, back);
	glPopMatrix();
}

// SCENE # 2
void fx_flake_tunnel()
{
	start_frame_3D();

	struct Gradient* grad = select_gradient();
	if (grad == NULL)
	{
		return;
	}

	morph_flake(&flake);
	flake.recursion_level = 4;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
	glDisable(GL_DEPTH_TEST);

	glPushMatrix();
	translate_by_rocket(0.0f, 0.0f);
	rotate_by_rocket();
	scale_by_rocket(false);


	float2 center = {0.0f, 0.0f};

	int shapes = (int)get_from_rocket(track_tunnel_shapes);
	float gradient_step = get_from_rocket(track_tunnel_gradient_step);

	float base_color = get_from_rocket(track_gradient_offset);
	float scale_step = get_from_rocket(track_tunnel_scale_step) / 100.0f;
	float rotation_step = get_from_rocket(track_tunnel_rotation_step);
	screenprintf("Tunnel shapes %d", shapes);
	for(int f = 0; f < shapes; f++)
	{
		Gradient_glColor(grad, base_color + gradient_step * f);
		glPushMatrix();
			glRotatef(rotation_step * f, 0.0f, 0.0f, 1.0f);
			float flake_scale = scale - scale_step * (f+1);
			screenprintf("Flake scale %d: %.2f", f, flake_scale);
			glScalef(flake_scale, flake_scale, 1.0f);
			Mesh_Draw(&flake_mesh_recursion4, DrawTriangles);
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
	start_frame_2D();
	morph_flake(&flake);
	flake.recursion_level = 4;
	float old_radius = flake.radius;
	float scale = get_from_rocket(track_scale_xyz);
	flake.radius = scale;
	KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
	flake.radius = old_radius;
	glPushMatrix();
		translate_by_rocket(center_x, center_y);
		glRotatef( get_from_rocket(track_rotation_z), 0.0f, 0.0f, 1.0f );

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

void reset_flake_on_scene_change(int scene)
{
	static int prev_scene = 0;
	if (scene != prev_scene)
	{
		KochFlake_SetMorphToDefault(&flake);
		KochFlake_WriteToMesh(&flake, &flake_mesh_recursion4);
		prev_scene = scene;
	}

}

void fx_zen_ending()
{
	start_frame_ortho_3D();

	{ // BG GRADIENT

		glPushMatrix();

			struct Gradient* bg_grad = select_gradient();
			// Make sure gradient stays behind bunny
			glTranslatef(0.0f,
						 0.2f,
						 -FAR_PLANE + 10.0f);

			float grad_size = get_from_rocket(track_gradient_size);
			glScalef(1.0f/grad_size * 2, 1.0/grad_size * 2, 1.0f);

			GradientTexture_DrawGradient(bg_grad, GradientCutout,
										 grad_size,
					get_from_rocket(track_gradient_offset));

		glPopMatrix();
	}

	{// Golden bunny

	glPushMatrix();
	glTranslatef(
		get_from_rocket(track_translate_x),
		get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z));

		rotate_by_rocket();
		scale_by_rocket(true);

		glColor3f(1.0f, 1.0f, 1.0f);
			struct Mesh* stfrd = &bunny_mesh.mesh;
			Mesh_GenerateMatcapUVs(stfrd);
			Mesh_EnableAttribute(stfrd, AttributeTexcoord);
			glEnable(GL_TEXTURE_2D);

			struct GradientTexture* material = select_matcap(track_matcap_index);
			glBindTexture(GL_TEXTURE_2D, material->gl_texture_name);

			Bunny_Draw_mesh(&bunny_mesh, DrawTriangles);

			glDisable(GL_TEXTURE_2D);
			Mesh_DisableAttribute(stfrd, AttributeTexcoord);

	glPopMatrix();
	}


	{	// Zen bunny

	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
		float2 bunny_pos = {get_from_rocket(track_bunny_x), get_from_rocket(track_bunny_y)};
		float bunny_size = get_from_rocket(track_bunny_size);
		float offset = get_from_rocket(track_gradient_offset);
		float bunny_rot = get_from_rocket(track_rotation_z);
		float2 gradient_pos = {get_from_rocket(track_translate_x), get_from_rocket(track_translate_y)};


		struct GradientTexture* text = select_texture();
		if (text == NULL)
		{
			return;
		}
		glPushMatrix();

			glTranslatef(bunny_pos.x, bunny_pos.y/100.0f, 0.0f);
			glScalef(1.0f, 1.0f, 1.0f);

			GradientTexture_DrawTexture(text, bunny_size);
		glPopMatrix();
	glDisable(GL_BLEND);
	}
}

void update_timing(int scene)
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
	screenprint_start_frame();
	screenprint_set_scale(2.0f);

	center_x = ctoy_frame_buffer_width()/2;
	center_y = ctoy_frame_buffer_height()/2;

	clear_screen();
	start_frame_2D();
	float scene_number = get_from_rocket(track_scene);
	int scene = (int)scene_number;

	screenprint("I am all ears");
	screenprintf("Active scene %.0f", scene_number);
	screenprintf("----------------", scene_number);

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
	update_timing(scene);
	reset_flake_on_scene_change(scene);
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
		case 7:
			// TODO Many bunnies
			// Bunny flock
			break;

		case 8:
			// Evangelion bunny
			fx_eva_bunny();
			break;

		case 9:
			// Ending scene
			fx_zen_ending();
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

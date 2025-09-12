
// Common
static int track_scene;	// Currently active scene
static int track_translate_x; // glTranslate x for effect
static int track_translate_y; // glTranslate y for effect
static int track_translate_z; // glTranslate z for effect
static int track_scale_xyz;	  // glScale x,y,z for effect
static int track_rotation_x;	  // glRotate x,y,z for effect
static int track_rotation_y;	  // glRotate x,y,z for effect
static int track_rotation_z;	  // glRotate x,y,z for effect
static int track_background_color;

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

// Flake wheel tracks
static int track_flake_wheel_radius;
static int track_flake_wheel_outer_radius;
static int track_flake_wheel_shape_rotation;
static int track_flake_wheel_pattern_rotation;
static int track_flake_wheel_outer_pattern_rotation;
static int track_flake_wheel_ring_color;
static int track_flake_wheel_shape_color;

// Gosper curve
static int track_gosper_segments;
static int track_gosper_follow_mix; // 0: no follow 1: full follow
static int track_gosper_width;
static int track_gosper_grad_step; // [0,100] how much to advance in gradient per segment

// Rotation illusion
static int track_rotation_illusion_progress;

// Stanford bunny
static int track_stanford_light_on;
static int track_stanford_light_dir; // Rotation on X-axis
static int track_stanford_light_color; // Pick color from active gradient
static int track_stanford_light_intensity; //
static int track_stanford_light_ambient_color; // Ambient color from gradient
static int track_stanford_light_ambient_intensity; // Ambient color intensity
static int track_stanford_triangle_percentage;

// Matcap bunny
static int track_matcap_index;
static int track_matcap_index2;
static int track_matcap_index3;
static int track_matcap_amount; // How many matcap bunnies and scissors to do : max 3

static int track_matcap_alpha; // Alpha of first matcap
static int track_matcap_base_on;

// TODO Scissors rectangles
static int track_scissor_1_cut;
static int track_scissor_1_left;
static int track_scissor_1_right;
static int track_scissor_1_top;
static int track_scissor_1_bottom;

static int track_scissor_2_left;
static int track_scissor_2_right;
static int track_scissor_2_top;
static int track_scissor_2_bottom;

static int track_scissor_3_left;
static int track_scissor_3_right;
static int track_scissor_3_top;
static int track_scissor_3_bottom;

// Variables




static void init_rocket_tracks(void)
{
	// Timing
	set_BPM(bpm);
	set_RPB(rpb);
    row_rate = (bpm / 60.0) * rpb;
	track_row = add_to_rocket("row");

	// General
	track_scene = add_to_rocket("scene");
	track_translate_x = add_to_rocket("translate_x");
	track_translate_y = add_to_rocket("translate_y");
	track_translate_z = add_to_rocket("translate_z");
	track_scale_xyz = add_to_rocket("scale_xyz");
	track_rotation_x = add_to_rocket("rotation_x");
	track_rotation_y = add_to_rocket("rotation_y");
	track_rotation_z = add_to_rocket("rotation_z");

	// Background color : from active gradient
	track_background_color = add_to_rocket("background_col");


	// Gradient
	track_gradient_mode = add_to_rocket("gradient_mode");
	track_gradient_shape = add_to_rocket("gradient_shape");
	track_gradient_offset = add_to_rocket("gradient_offset");
	track_gradient_repeat = add_to_rocket("gradient_repeat");
	track_gradient_index = add_to_rocket("gradient_index");
	track_gradient_size = add_to_rocket("gradient_size");

	// Bunnies and logos
	track_bunny_index = add_to_rocket("bunny_index");
	track_bunny_x = add_to_rocket("bunny_x");
	track_bunny_y = add_to_rocket("bunny_y");
	track_bunny_size = add_to_rocket("bunny_size");

	// Koch Flake shape
	track_flake_ratio_off = add_to_rocket("flake_ratio_off");
	track_flake_angle_off = add_to_rocket("flake_angle_off");
	track_flake_extrusion_off = add_to_rocket("flake_extrusion_off");

	// Flake tunnel tracks
	track_tunnel_shapes = add_to_rocket("tunnel_shapes");
	track_tunnel_scale_step = add_to_rocket("tunnel_scale_step");
	track_tunnel_rotation_step = add_to_rocket("tunnel_rotation_step");
	track_tunnel_gradient_step = add_to_rocket("tunnel_gradient_step");

    // Flake wheel tracks
    track_flake_wheel_radius = add_to_rocket("wheel_radius");
    track_flake_wheel_outer_radius = add_to_rocket("wheel_outer_radius");
    track_flake_wheel_shape_rotation = add_to_rocket("wheel_shape_rot");
    track_flake_wheel_pattern_rotation = add_to_rocket("wheel_ptrn_rot");
    track_flake_wheel_outer_pattern_rotation = add_to_rocket("wheel_outer_ptrn_rot");
    track_flake_wheel_ring_color = add_to_rocket("wheel_ring_color");
    track_flake_wheel_shape_color = add_to_rocket("wheel_shape_color");

	// Gosper curve tracks
	track_gosper_follow_mix = add_to_rocket("gosper_follow");
	track_gosper_segments = add_to_rocket("gosper_segments");
	track_gosper_width = add_to_rocket("gosper_width");
	track_gosper_grad_step = add_to_rocket("gosper_grad_step");

	// Rotation illusion
	track_rotation_illusion_progress = add_to_rocket("illusion_progress");


	// Stanford
	track_stanford_light_on = add_to_rocket("stfrd_light_on");
	track_stanford_light_dir = add_to_rocket("stfrd_light_dir");
    track_stanford_light_color = add_to_rocket("stfrd_light_color");
    track_stanford_light_ambient_color = add_to_rocket("stfrd_light_amb_color");
    track_stanford_light_ambient_intensity = add_to_rocket("stfrd_amb_int");
    track_stanford_light_intensity = add_to_rocket("stfrd_light_intensity");
	track_stanford_triangle_percentage = add_to_rocket("stfrd_tri_percentage");


	// Matcap & scissors
	track_matcap_amount = add_to_rocket("matcap_amount");
	track_matcap_alpha = add_to_rocket("matcap1_alpha");

	track_matcap_base_on = add_to_rocket("matcap_base_on");

	track_matcap_index = add_to_rocket("matcap_index");
	track_matcap_index2 = add_to_rocket("matcap_index2");
	track_matcap_index3 = add_to_rocket("matcap_index3");

	track_scissor_1_cut = add_to_rocket("scissors_1_cut");
	track_scissor_1_left= add_to_rocket("scissors_1_left");
	track_scissor_1_right= add_to_rocket("scissors_1_right");
	track_scissor_1_top= add_to_rocket("scissors_1_top");
	track_scissor_1_bottom= add_to_rocket("scissors_1_bottom");

	track_scissor_2_left= add_to_rocket("scissors_2_left");
	track_scissor_2_right= add_to_rocket("scissors_2_right");
	track_scissor_2_top= add_to_rocket("scissors_2_top");
	track_scissor_2_bottom= add_to_rocket("scissors_2_bottom");

	track_scissor_3_left= add_to_rocket("scissors_3_left");
	track_scissor_3_right= add_to_rocket("scissors_3_right");
	track_scissor_3_top= add_to_rocket("scissors_3_top");
	track_scissor_3_bottom= add_to_rocket("scissors_3_bottom");
}

#ifndef MAIN_UTIL_FUNCTIONS_H
#define MAIN_UTIL_FUNCTIONS_H

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

void clear_screen( void )
{
    float bg_stop = get_from_rocket(track_background_color);
    if (bg_stop > 0.0f)
    {
        struct Gradient* activeGrad = select_gradient();
        color3 bg = Gradient_GetColor(activeGrad, bg_stop);
        glClearColor(bg.r, bg.g, bg.b, 0.0f);
    }
	else
	{
		glClearColor(0,0,0,0);
	}
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

void translate_by_rocket(float center_x, float center_y)
{
	glTranslatef(
		center_x + get_from_rocket(track_translate_x),
		center_y + get_from_rocket(track_translate_y),
		get_from_rocket(track_translate_z)
	);
}

void rotate_by_rocket(void )
{
	glRotatef(get_from_rocket(track_rotation_x), 1.0f, 0.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_y), 0.0f, 1.0f, 0.0f);
	glRotatef(get_from_rocket(track_rotation_z), 0.0f, 0.0f, 1.0f);
}

void scale_by_rocket(bool do_z_scale)
{
		const float scale = get_from_rocket(track_scale_xyz);
        float scale_z = scale;
        if (do_z_scale == false)
        {
            scale_z = 1.0f;
        }
		glScalef(scale, scale, scale_z);
}

#endif

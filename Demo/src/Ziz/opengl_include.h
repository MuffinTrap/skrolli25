#ifndef OPENGL_INCLUDE_H
#define OPENGL_INCLUDE_H
#include <ctoy.h>
#include <string.h>

#include <GL_macros.h>

#ifdef __APPLE__
	#include <gl2.h>
#endif

#ifdef __linux__
	#include <GL/gl.h>
	#include <GL/glu.h>
    #include <GL/glut.h>
#endif

#ifdef N64
    #include <libdragon.h>
    #include <GL/gl.h>
    #include <GL/glu.h> 
    #include <GL/gl_integration.h>
#endif

#endif

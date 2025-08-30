#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

// Define GLfloat if not already defined
#ifndef GLfloat
#define GLfloat float
#endif

// Define vec3 type if not already defined
#ifndef VEC3_DEFINED
#define VEC3_DEFINED
typedef float vec3[3];
#endif

// Global frustum planes
static float frustum[6][4];

// Function to test if a sphere is inside the frustum
int SphereInFrustum(float x, float y, float z, float radius) {
    for(int p = 0; p < 6; p++) {
        if(frustum[p][0] * x + frustum[p][1] * y + frustum[p][2] * z + frustum[p][3] <= -radius) {
            return 0; // Sphere is outside this plane
        }
    }
    return 1; // Sphere is inside all planes
}

// Create perspective projection matrix
void perspective(GLfloat* mat, GLfloat fov, GLfloat aspect, GLfloat zNear, GLfloat zFar) {
    zFar *= 8.0f;  // TODO: THERE'S SOME BUG HERE WITH ZFAR DUNNO - TOO BUSY, FIX LATER #YOLO
    GLfloat f = 1.0f / tan(fov * M_PI / 360.0f);
    mat[0] = f / aspect;
    mat[1] = 0.0f;
    mat[2] = 0.0f;
    mat[3] = 0.0f;
    mat[4] = 0.0f;
    mat[5] = f;
    mat[6] = 0.0f;
    mat[7] = 0.0f;
    mat[8] = 0.0f;
    mat[9] = 0.0f;
    mat[10] = (zFar + zNear) / (zNear - zFar);
    mat[11] = -1.0f;
    mat[12] = 0.0f;
    mat[13] = 0.0f;
    mat[14] = (2.0f * zFar * zNear) / (zNear - zFar);
    mat[15] = 0.0f;
}

// Create look-at view matrix
void lookAt(GLfloat* mat, vec3 eye, vec3 center, vec3 up) {
    GLfloat f[3], r[3], u[3];
    
    f[0] = center[0] - eye[0];
    u[0] = up[0];
    f[1] = center[1] - eye[1];
    u[1] = up[1];
    f[2] = center[2] - eye[2];
    u[2] = up[2];
    
    GLfloat fMag = sqrt(f[0] * f[0] + f[1] * f[1] + f[2] * f[2]);
    for (int i = 0; i < 3; i++) {
        f[i] /= fMag;
        u[i] /= sqrt(u[0] * u[0] + u[1] * u[1] + u[2] * u[2]);
    }
    
    r[0] = u[1] * f[2] - u[2] * f[1];
    r[1] = u[2] * f[0] - u[0] * f[2];
    r[2] = u[0] * f[1] - u[1] * f[0];
    
    GLfloat rMag = sqrt(r[0] * r[0] + r[1] * r[1] + r[2] * r[2]);
    for (int i = 0; i < 3; i++) {
        r[i] /= rMag;
    }
    
    u[0] = f[1] * r[2] - f[2] * r[1];
    u[1] = f[2] * r[0] - f[0] * r[2];
    u[2] = f[0] * r[1] - f[1] * r[0];
    
    mat[0] = r[0];
    mat[1] = u[0];
    mat[2] = -f[0];
    mat[3] = 0.0f;
    
    mat[4] = r[1];
    mat[5] = u[1];
    mat[6] = -f[1];
    mat[7] = 0.0f;
    
    mat[8] = r[2];
    mat[9] = u[2];
    mat[10] = -f[2];
    mat[11] = 0.0f;
    
    mat[12] = -r[0] * eye[0] - r[1] * eye[1] - r[2] * eye[2];
    mat[13] = -u[0] * eye[0] - u[1] * eye[1] - u[2] * eye[2];
    mat[14] = f[0] * eye[0] + f[1] * eye[1] + f[2] * eye[2];
    mat[15] = 1.0f;
}

// Extract frustum planes from camera parameters
void ExtractFrustum(vec3 cameraPosition, vec3 cameraLookAt, vec3 cameraUp, float fov, float aspect, float near, float far) {
    float proj[16];
    float modl[16];
    float clip[16];
    float t;
    
    perspective(proj, fov, aspect, near, far);
    lookAt(modl, cameraPosition, cameraLookAt, cameraUp);
    
    /* Combine the two matrices (multiply projection by modelview) */
    clip[ 0] = modl[ 0] * proj[ 0] + modl[ 1] * proj[ 4] + modl[ 2] * proj[ 8] + modl[ 3] * proj[12];
    clip[ 1] = modl[ 0] * proj[ 1] + modl[ 1] * proj[ 5] + modl[ 2] * proj[ 9] + modl[ 3] * proj[13];
    clip[ 2] = modl[ 0] * proj[ 2] + modl[ 1] * proj[ 6] + modl[ 2] * proj[10] + modl[ 3] * proj[14];
    clip[ 3] = modl[ 0] * proj[ 3] + modl[ 1] * proj[ 7] + modl[ 2] * proj[11] + modl[ 3] * proj[15];
    clip[ 4] = modl[ 4] * proj[ 0] + modl[ 5] * proj[ 4] + modl[ 6] * proj[ 8] + modl[ 7] * proj[12];
    clip[ 5] = modl[ 4] * proj[ 1] + modl[ 5] * proj[ 5] + modl[ 6] * proj[ 9] + modl[ 7] * proj[13];
    clip[ 6] = modl[ 4] * proj[ 2] + modl[ 5] * proj[ 6] + modl[ 6] * proj[10] + modl[ 7] * proj[14];
    clip[ 7] = modl[ 4] * proj[ 3] + modl[ 5] * proj[ 7] + modl[ 6] * proj[11] + modl[ 7] * proj[15];
    clip[ 8] = modl[ 8] * proj[ 0] + modl[ 9] * proj[ 4] + modl[10] * proj[ 8] + modl[11] * proj[12];
    clip[ 9] = modl[ 8] * proj[ 1] + modl[ 9] * proj[ 5] + modl[10] * proj[ 9] + modl[11] * proj[13];
    clip[10] = modl[ 8] * proj[ 2] + modl[ 9] * proj[ 6] + modl[10] * proj[10] + modl[11] * proj[14];
    clip[11] = modl[ 8] * proj[ 3] + modl[ 9] * proj[ 7] + modl[10] * proj[11] + modl[11] * proj[15];
    clip[12] = modl[12] * proj[ 0] + modl[13] * proj[ 4] + modl[14] * proj[ 8] + modl[15] * proj[12];
    clip[13] = modl[12] * proj[ 1] + modl[13] * proj[ 5] + modl[14] * proj[ 9] + modl[15] * proj[13];
    clip[14] = modl[12] * proj[ 2] + modl[13] * proj[ 6] + modl[14] * proj[10] + modl[15] * proj[14];
    clip[15] = modl[12] * proj[ 3] + modl[13] * proj[ 7] + modl[14] * proj[11] + modl[15] * proj[15];
    
    /* Extract the numbers for the RIGHT plane */
    frustum[0][0] = clip[ 3] - clip[ 0];
    frustum[0][1] = clip[ 7] - clip[ 4];
    frustum[0][2] = clip[11] - clip[ 8];
    frustum[0][3] = clip[15] - clip[12];
    /* Normalize the result */
    t = sqrt( frustum[0][0] * frustum[0][0] + frustum[0][1] * frustum[0][1] + frustum[0][2] * frustum[0][2] );
    frustum[0][0] /= t;
    frustum[0][1] /= t;
    frustum[0][2] /= t;
    frustum[0][3] /= t;
    
    /* Extract the numbers for the LEFT plane */
    frustum[1][0] = clip[ 3] + clip[ 0];
    frustum[1][1] = clip[ 7] + clip[ 4];
    frustum[1][2] = clip[11] + clip[ 8];
    frustum[1][3] = clip[15] + clip[12];
    /* Normalize the result */
    t = sqrt( frustum[1][0] * frustum[1][0] + frustum[1][1] * frustum[1][1] + frustum[1][2] * frustum[1][2] );
    frustum[1][0] /= t;
    frustum[1][1] /= t;
    frustum[1][2] /= t;
    frustum[1][3] /= t;
    
    /* Extract the BOTTOM plane */
    frustum[2][0] = clip[ 3] + clip[ 1];
    frustum[2][1] = clip[ 7] + clip[ 5];
    frustum[2][2] = clip[11] + clip[ 9];
    frustum[2][3] = clip[15] + clip[13];
    /* Normalize the result */
    t = sqrt( frustum[2][0] * frustum[2][0] + frustum[2][1] * frustum[2][1] + frustum[2][2] * frustum[2][2] );
    frustum[2][0] /= t;
    frustum[2][1] /= t;
    frustum[2][2] /= t;
    frustum[2][3] /= t;
    
    /* Extract the TOP plane */
    frustum[3][0] = clip[ 3] - clip[ 1];
    frustum[3][1] = clip[ 7] - clip[ 5];
    frustum[3][2] = clip[11] - clip[ 9];
    frustum[3][3] = clip[15] - clip[13];
    /* Normalize the result */
    t = sqrt( frustum[3][0] * frustum[3][0] + frustum[3][1] * frustum[3][1] + frustum[3][2] * frustum[3][2] );
    frustum[3][0] /= t;
    frustum[3][1] /= t;
    frustum[3][2] /= t;
    frustum[3][3] /= t;
    
    /* Extract the FAR plane */
    frustum[4][0] = clip[ 3] - clip[ 2];
    frustum[4][1] = clip[ 7] - clip[ 6];
    frustum[4][2] = clip[11] - clip[10];
    frustum[4][3] = clip[15] - clip[14];
    /* Normalize the result */
    t = sqrt( frustum[4][0] * frustum[4][0] + frustum[4][1] * frustum[4][1] + frustum[4][2] * frustum[4][2] );
    frustum[4][0] /= t;
    frustum[4][1] /= t;
    frustum[4][2] /= t;
    frustum[4][3] /= t;
    
    /* Extract the NEAR plane */
    frustum[5][0] = clip[ 3] + clip[ 2];
    frustum[5][1] = clip[ 7] + clip[ 6];
    frustum[5][2] = clip[11] + clip[10];
    frustum[5][3] = clip[15] + clip[14];
    /* Normalize the result */
    t = sqrt( frustum[5][0] * frustum[5][0] + frustum[5][1] * frustum[5][1] + frustum[5][2] * frustum[5][2] );
    frustum[5][0] /= t;
    frustum[5][1] /= t;
    frustum[5][2] /= t;
    frustum[5][3] /= t;
}

// Test if a bounding box is entirely visible
int IsBoundingBoxEntirelyVisible(float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) {
    for (int p = 0; p < 6; p++) {
        float a = frustum[p][0];
        float b = frustum[p][1];
        float c = frustum[p][2];
        float d = frustum[p][3];

        int isFullyVisible = 1; // Assume the bounding box is fully visible

        for (int corner = 0; corner < 8; corner++) {
            float x = (corner & 1) ? max_x : min_x;
            float y = (corner & 2) ? max_y : min_y;
            float z = (corner & 4) ? max_z : min_z;

            if (a * x + b * y + c * z + d > 0) {
                isFullyVisible = 0;
                break; // Bounding box is not fully visible
            }
        }

        if (!isFullyVisible) {
            return 0; // Bounding box is not entirely visible
        }
    }

    return 1; // Bounding box is entirely visible in all planes
}

// Function to perform frustum culling
int IsBoundingBoxVisible(float min_x, float min_y, float min_z, float max_x, float max_y, float max_z) {
    int p;
    float dot;
    
    for (p = 0; p < 6; p++) {
        float a = frustum[p][0];
        float b = frustum[p][1];
        float c = frustum[p][2];
        float d = frustum[p][3];
        
        dot = a * min_x + b * min_y + c * min_z + d;
        if (dot > 0) continue;

        dot = a * max_x + b * min_y + c * min_z + d;
        if (dot > 0) continue;

        dot = a * min_x + b * max_y + c * min_z + d;
        if (dot > 0) continue;

        dot = a * max_x + b * max_y + c * min_z + d;
        if (dot > 0) continue;

        dot = a * min_x + b * min_y + c * max_z + d;
        if (dot > 0) continue;

        dot = a * max_x + b * min_y + c * max_z + d;
        if (dot > 0) continue;

        dot = a * min_x + b * max_y + c * max_z + d;
        if (dot > 0) continue;

        dot = a * max_x + b * max_y + c * max_z + d;
        if (dot > 0) continue;

        return 0;
    }
    
    return 1;
}

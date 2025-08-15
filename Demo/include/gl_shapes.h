#ifndef GL_SHAPES_H
#define GL_SHAPES_H

#include "m_math.h"
#ifndef N64
#include "gl2.h"
#else
#include <libdragon.h>
#include <GL/gl.h>
#include <GL/glu.h>
#include <GL/gl_integration.h>
#endif

#define MAX_VERTICES 4096
#define MAX_INDICES 8192 

// Batch rendering buffers
extern float2 positions[MAX_VERTICES];
extern float4 colors[MAX_VERTICES];
extern GLushort indices[MAX_INDICES];
extern int vertex_count;
extern int index_count;

// Screen constants
#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define PI 3.14159265f

// Reset batch for new frame
void gl_shapes_begin() {
    vertex_count = 0;
    index_count = 0;
    glDisable(GL_CULL_FACE);
}

// Draw entire batch
void gl_shapes_end() {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    glEnableClientState(GL_VERTEX_ARRAY);
    glEnableClientState(GL_COLOR_ARRAY);
    
    glVertexPointer(2, GL_FLOAT, 0, positions);
    glColorPointer(4, GL_FLOAT, 0, colors);
    
    glDrawElements(GL_TRIANGLES, index_count, GL_UNSIGNED_SHORT, indices);
    
    glDisableClientState(GL_COLOR_ARRAY);
    glDisableClientState(GL_VERTEX_ARRAY);
    glDisable(GL_BLEND);
}

// New texture control functions
void gl_shapes_enable_textures(bool enabled);
void gl_shapes_set_texture(GLuint tex_id);
void gl_shapes_disable_textures(void);
int gl_shapes_get_vertex_count(void);

int calculateOptimalSegments(float radius, float min, float max) {
    if (radius <= 0) return (int)min;
    float px = radius;
    int seg = (int)(PI * px);
    return (seg < min ? min : seg > max ? max : seg) / 4;
}

void drawCircle(float2 pos, float radius, float4 color) {
    int seg = calculateOptimalSegments(radius, 8, 64) * 1.5f;
    int needed_verts = seg + 2;
    int needed_indices = seg * 3;
    
    // Check batch capacity
    if(vertex_count + needed_verts > MAX_VERTICES || 
       index_count + needed_indices > MAX_INDICES) return;
       
    // Center vertex
    positions[vertex_count] = pos;
    colors[vertex_count] = color;
    int center_idx = vertex_count++;
    
    // Perimeter vertices
    int first_rim = vertex_count;
    for(int i = 0; i <= seg; i++) {
        float a = i * (2 * PI / seg);
        positions[vertex_count] = (float2){
            pos.x + cosf(a) * radius,
            pos.y + sinf(a) * radius
        };
        colors[vertex_count] = color;
        vertex_count++;
    }
    
    // Indices
    for(int i = 0; i < seg; i++) {
        indices[index_count++] = center_idx;
        indices[index_count++] = first_rim + i;
        indices[index_count++] = first_rim + ((i+1) % seg);
    }
}
void drawCircleRing(float2 pos, float radius, float thickness, float4 color, int filled) {
    if(filled) {
        drawCircle( pos, radius + thickness/2, color);
        return;
    }
    // Calculate optimal number of segments based on outer radius
    int seg = calculateOptimalSegments(radius, 12, 96);
    int needed_verts = seg * 2;
    int needed_indices = seg * 6;
    
    // Check batch capacity
    if(vertex_count + needed_verts > MAX_VERTICES || 
       index_count + needed_indices > MAX_INDICES) return;
    
    // Generate vertices for outer and inner circles
    const float inner_radius = radius - thickness;
    int outer_start = vertex_count;
    
    // Outer circle vertices
    for(int i = 0; i < seg; i++) {
        float angle = i * (2 * PI / seg);
        positions[vertex_count] = (float2){
            pos.x + cosf(angle) * radius,
            pos.y + sinf(angle) * radius
        };
        colors[vertex_count++] = color;
    }
    
    // Inner circle vertices
    int inner_start = vertex_count;
    for(int i = 0; i < seg; i++) {
        float angle = i * (2 * PI / seg);
        positions[vertex_count] = (float2){
            pos.x + cosf(angle) * inner_radius,
            pos.y + sinf(angle) * inner_radius
        };
        colors[vertex_count++] = color;
    }
    
    // Create indices for ring segments
    for(int i = 0; i < seg; i++) {
        int next_i = (i + 1) % seg;
        
        // Outer and inner indices for current segment
        int o0 = outer_start + i;
        int o1 = outer_start + next_i;
        int i0 = inner_start + i;
        int i1 = inner_start + next_i;
        
        // Add two triangles per segment
        indices[index_count++] = o0;
        indices[index_count++] = o1;
        indices[index_count++] = i0;
        
        indices[index_count++] = o1;
        indices[index_count++] = i1;
        indices[index_count++] = i0;
    }
}

void drawCapsule(float2 a, float2 b, float radius, float4 color) {
    int seg = calculateOptimalSegments(radius, 6, 32);
    float2 dir = {b.x - a.x, b.y - a.y};
    float len = sqrtf(dir.x*dir.x + dir.y*dir.y);
    if(len < 0.001f) return;
    
    float2 norm = {-dir.y/len, dir.x/len};
    int needed_verts = (seg + 1) * 2;  // Corrected vertex count
    int needed_indices = seg * 6;
    
    if(vertex_count + needed_verts > MAX_VERTICES ||
       index_count + needed_indices > MAX_INDICES) return;
    
    int start = vertex_count;
    
    // Generate vertices (paired top/bottom for each segment)
    for(int i = 0; i <= seg; i++) {
        float angle = i * (PI / seg);
        float2 off = {cosf(angle)*radius, -sinf(angle)*radius};

        
        // Top vertex (b end)
        positions[vertex_count] = (float2){
            b.x + off.x*norm.x - off.y*dir.x/len,
            b.y + off.x*norm.y - off.y*dir.y/len
        };
        colors[vertex_count++] = color;
        
        // Bottom vertex (a end)
        positions[vertex_count] = (float2){
            a.x + off.x*norm.x + off.y*dir.x/len,
            a.y + off.x*norm.y + off.y*dir.y/len
        };
        colors[vertex_count++] = color;
    }
    
    // Generate indices (triangle strip pattern)
    for(int i = 0; i < seg; i++) {
        int base = start + i*2;
        // First triangle (top->bottom->next top)
        indices[index_count++] = base;
        indices[index_count++] = base + 1;
        indices[index_count++] = base + 2;
        
        // Second triangle (next top->bottom->next bottom)
        indices[index_count++] = base + 2;
        indices[index_count++] = base + 1;
        indices[index_count++] = base + 3;
    }
}
void drawRoundBox(float2 pos, float2 size, float radius, float4 color, int fill) {
    float2 pts[4] = {
        {pos.x + size.x - radius, pos.y + size.y - radius},
        {pos.x - size.x + radius, pos.y + size.y - radius},
        {pos.x - size.x + radius, pos.y - size.y + radius},
        {pos.x + size.x - radius, pos.y - size.y + radius}
    };
    if(fill){ 
        int start = vertex_count;
        for(int i = 0; i < 4; i++) {
            positions[vertex_count] = pts[i];
            colors[vertex_count++] = color;
        }
        
        indices[index_count++] = start;
        indices[index_count++] = start+1;
        indices[index_count++] = start+2;
        indices[index_count++] = start+3;
        indices[index_count++] = start+0;
        indices[index_count++] = start+2;
    }
    // Generate vertices
    for(int c = 0; c < 4; c++) {
        drawCapsule(pts[c % 4], pts[(c+1) % 4], radius, color);
    }
}


void drawTriangle(float2 pos, float angle, float size, float thickness, float4 color, int fill) {
    float h = size * (sqrtf(3.0f)/2.0f);
    float2 pts[3] = {
        {pos.x, pos.y + h},
        {pos.x - size/2, pos.y},
        {pos.x + size/2, pos.y}
    };
    
    // Rotate
    float ca = cosf(angle);
    float sa = sinf(angle);
    for(int i = 0; i < 3; i++) {
        float x = pts[i].x - pos.x;
        float y = pts[i].y - pos.y;
        pts[i].x = pos.x + x*ca - y*sa;
        pts[i].y = pos.y + x*sa + y*ca;
    }
    
    if(vertex_count + 3 > MAX_VERTICES || index_count + 3 > MAX_INDICES) return;
    if(fill){ 
        int start = vertex_count;
        for(int i = 0; i < 3; i++) {
            positions[vertex_count] = pts[i];
            colors[vertex_count++] = color;
        }
        
        indices[index_count++] = start;
        indices[index_count++] = start+1;
        indices[index_count++] = start+2;
    }
    drawCapsule( pts[0], pts[1], thickness, color);
    drawCapsule( pts[1], pts[2], thickness, color);
    drawCapsule( pts[2], pts[0], thickness, color);
}
// Computes perpendicular (radius-scaled) vector between two points
static float2 compute_perp(float2 a, float2 b, float radius) {
    float2 delta = { b.x - a.x, b.y - a.y };
    float len = sqrtf(delta.x * delta.x + delta.y * delta.y);
    if (len < 0.001f) return (float2){0, 0};
    float2 dir = { delta.x / len, delta.y / len };
    return (float2){ -dir.y * radius, dir.x * radius };
}
void drawSegment(float2 a, float2 b, float radius, float4 color, 
                 float2 start_top, float2 start_bottom, 
                 float2 *end_top, float2 *end_bottom) {
    float2 delta = (float2){b.x - a.x, b.y - a.y};
    float len = sqrtf(delta.x*delta.x + delta.y*delta.y);
    if (len < 0.001f) {
        *end_top = start_top;
        *end_bottom = start_bottom;
        drawCircle(a, radius, color);
        return;
    }
    
    // Direction vectors
    float2 dir = (float2){delta.x / len, delta.y / len};
    float2 perp = (float2){-dir.y * radius, dir.x * radius};
    float2 b_top = (float2){b.x + perp.x, b.y + perp.y};
    float2 b_bottom = (float2){b.x - perp.x, b.y - perp.y};
    
    // Check batch capacity (4 vertices, 6 indices)
    if (vertex_count + 4 > MAX_VERTICES || index_count + 6 > MAX_INDICES) {
        *end_top = start_top;
        *end_bottom = start_bottom;
        return;
    }
    
    // Add vertices
    int start_idx = vertex_count;
    positions[vertex_count] = start_top;    colors[vertex_count++] = color;
    positions[vertex_count] = start_bottom; colors[vertex_count++] = color;
    positions[vertex_count] = b_bottom;     colors[vertex_count++] = color;
    positions[vertex_count] = b_top;        colors[vertex_count++] = color;
    
    // Add indices
    indices[index_count++] = start_idx;
    indices[index_count++] = start_idx + 1;
    indices[index_count++] = start_idx + 2;
    
    indices[index_count++] = start_idx;
    indices[index_count++] = start_idx + 2;
    indices[index_count++] = start_idx + 3;
    
    *end_top = b_top;
    *end_bottom = b_bottom;
}

void drawBezierRibbon(float2 a, float2 control, float2 b, float radius, float4 color, 
                      float2 start_top, float2 start_bottom, 
                      float2 *end_top, float2 *end_bottom) {
    const int segments = 16;
    float2 prev_point = a;
    float2 prev_top = start_top;
    float2 prev_bottom = start_bottom;
    float2 prev_perp = (float2){prev_top.x - a.x, prev_top.y - a.y};
    
    for (int i = 1; i <= segments; i++) {
        float t = (float)i / segments;
        float u = 1.0f - t;
        
        // Compute point on Bézier curve
        float2 point = {
            u*u*a.x + 2*u*t*control.x + t*t*b.x,
            u*u*a.y + 2*u*t*control.y + t*t*b.y
        };
        
        // Compute perp for current segment
        float2 perp = compute_perp(prev_point, point, radius);
        float2 top = (float2){point.x + perp.x, point.y + perp.y};
        float2 bottom = (float2){point.x - perp.x, point.y - perp.y};
        
        if (vertex_count + 4 > MAX_VERTICES || index_count + 6 > MAX_INDICES) {
            *end_top = prev_top;
            *end_bottom = prev_bottom;
            return;
        }
        
        // Add vertices and indices for the segment
        int start_idx = vertex_count;
        if (i == 1) {
            // First segment adds all four vertices
            positions[vertex_count] = prev_top;    colors[vertex_count++] = color;
            positions[vertex_count] = prev_bottom; colors[vertex_count++] = color;
            positions[vertex_count] = top;         colors[vertex_count++] = color;
            positions[vertex_count] = bottom;      colors[vertex_count++] = color;
            
            indices[index_count++] = start_idx;
            indices[index_count++] = start_idx + 1;
            indices[index_count++] = start_idx + 2;
            
            indices[index_count++] = start_idx + 1;
            indices[index_count++] = start_idx + 3;
            indices[index_count++] = start_idx + 2;
        } else {
            // Subsequent segments reuse previous top and bottom as start vertices
            positions[vertex_count] = top;         colors[vertex_count++] = color;
            positions[vertex_count] = bottom;      colors[vertex_count++] = color;
            
            indices[index_count++] = start_idx - 2; // Previous top
            indices[index_count++] = start_idx - 1; // Previous bottom
            indices[index_count++] = start_idx;
            
            indices[index_count++] = start_idx - 1;
            indices[index_count++] = start_idx + 1;
            indices[index_count++] = start_idx;
        }
        
        prev_point = point;
        prev_top = top;
        prev_bottom = bottom;
    }
    
    *end_top = prev_top;
    *end_bottom = prev_bottom;
}

void drawCurvedPolyline(const float2* points, int num_points, int start, int count, float radius, float4 color) {
    if (start < 0 || start >= num_points || count <= 0) return;

    int end_index = start + count;
    if (end_index > num_points) end_index = num_points;
    int actual_count = end_index - start;

    if (actual_count < 2) return;

    // Draw caps
    drawCircle(points[start], radius, color);
    drawCircle(points[end_index - 1], radius, color);

    float2 current_top, current_bottom;
    int first_segment = 1;
    int i = start;

    while (i < end_index - 1) {
        if (i + 2 < end_index) { // Process Bézier triple
            float2 a = points[i];
            float2 control = points[i + 1];
            float2 b = points[i + 2];
            
            if (first_segment) {
                // Compute initial perpendicular based on first control point
                float2 initial_perp = compute_perp(a, control, radius);
                current_top = (float2){a.x + initial_perp.x, a.y + initial_perp.y};
                current_bottom = (float2){a.x - initial_perp.x, a.y - initial_perp.y};
                first_segment = 0;
            }
            
            drawBezierRibbon(a, control, b, radius, color, current_top, current_bottom, &current_top, &current_bottom);
            i += 2;
        } else { // Process remaining segment as line
            float2 a = points[i];
            float2 b = points[i + 1];
            
            if (first_segment) {
                // Compute initial perpendicular for the line
                float2 delta = {b.x - a.x, b.y - a.y};
                float len = sqrtf(delta.x*delta.x + delta.y*delta.y);
                if (len < 0.001f) {
                    i++;
                    continue;
                }
                float2 dir = {delta.x / len, delta.y / len};
                float2 perp = {-dir.y * radius, dir.x * radius};
                current_top = (float2){a.x + perp.x, a.y + perp.y};
                current_bottom = (float2){a.x - perp.x, a.y - perp.y};
                first_segment = 0;
            }
            
            drawSegment(a, b, radius, color, current_top, current_bottom, &current_top, &current_bottom);
            i++;
        }
    }
}

void drawArrow(float2 pos, float2 direction, float length, float thickness, float4 color) {
    // Normalize direction vector
    float dir_len = sqrtf(direction.x*direction.x + direction.y*direction.y);
    if (dir_len < 0.001f) return;
    float2 dir_norm = {direction.x/dir_len, direction.y/dir_len};
    
    // Calculate arrow shaft end point
    float2 end = {
        pos.x + dir_norm.x * length,
        pos.y + dir_norm.y * length
    };
    
    // Draw arrow shaft
    drawCapsule(pos, end, thickness, color);
    
    // Calculate arrowhead parameters
    float head_size = thickness * 3.0f;
    float theta = atan2f(dir_norm.y, dir_norm.x);
    float rotation_angle = theta - PI/2.0f; // Adjust for triangle orientation
    
    // Draw arrowhead triangle
    drawTriangle(end, rotation_angle, head_size, thickness, color, 1);
}

void drawCross(float2 pos, float size, float thickness, float4 color) {
    // Diagonal lines
    float2 a = {pos.x - size, pos.y - size};
    float2 b = {pos.x + size, pos.y + size};
    drawCapsule(a, b, thickness, color);
    
    float2 c = {pos.x + size, pos.y - size};
    float2 d = {pos.x - size, pos.y + size};
    drawCapsule(c, d, thickness, color);
}

void drawPlus(float2 pos, float size, float thickness, float4 color) {
    // Horizontal line
    float2 h_left = {pos.x - size, pos.y};
    float2 h_right = {pos.x + size, pos.y};
    drawCapsule(h_left, h_right, thickness, color);
    
    // Vertical line
    float2 v_top = {pos.x, pos.y + size};
    float2 v_bottom = {pos.x, pos.y - size};
    drawCapsule(v_top, v_bottom, thickness, color);
}

void drawStar(float2 pos, float outer_radius, float inner_radius, float thickness, float4 color, int fill) {
    const int num_points = 10;
    float2 points[num_points];
    
    // Generate star points (alternating between outer/inner radius)
    for(int i = 0; i < num_points; i++) {
        float angle = i * 36.0f * PI / 180.0f; // 36 degree steps
        float radius = (i % 2 == 0) ? outer_radius : inner_radius;
        points[i] = (float2){
            pos.x + cosf(angle) * radius,
            pos.y + sinf(angle) * radius
        };
    }

    if(fill) {
        // Add center vertex
        if(vertex_count + num_points + 1 > MAX_VERTICES) return;
        if(index_count + num_points * 3 > MAX_INDICES) return;
        
        int center_idx = vertex_count;
        positions[vertex_count] = pos;
        colors[vertex_count++] = color;
        
        // Add star perimeter vertices
        int points_start = vertex_count;
        for(int i = 0; i < num_points; i++) {
            positions[vertex_count] = points[i];
            colors[vertex_count++] = color;
        }
        
        // Create triangle indices (center, point, next_point)
        for(int i = 0; i < num_points; i++) {
            int next_i = (i + 1) % num_points;
            indices[index_count++] = center_idx;
            indices[index_count++] = points_start + i;
            indices[index_count++] = points_start + next_i;
        }
    } else {
        // Draw outline segments
        for(int i = 0; i < num_points; i++) {
            int next_i = (i + 1) % num_points;
            drawCapsule(points[i], points[next_i], thickness, color);
        }
    }
}
#endif
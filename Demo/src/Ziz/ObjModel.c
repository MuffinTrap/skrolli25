//#define CGLTF_IMPLEMENTATION
#include "ObjModel.h"
#include <wii_memory_functions.h>

#ifndef CGLTF_IMPLEMENTATION
    #define CGLTF_IMPLEMENTATION
    #include <cgltf.h>
#endif

#define MAX_MODELS 64

#ifdef N64
    static model64_t *models[MAX_MODELS];
    static char n64_model_paths[MAX_MODELS][256];  // Store full paths for N64 models
#else
    typedef struct {
        cgltf_data* data;  // We'll keep the loaded data alive
        char name[64];
    } ModelEntry;
static ModelEntry models[MAX_MODELS];
#endif 

static int model_count = 0;

#ifdef N64
    int findModelByPath(const char* path) {
        if (!path) return -1;

        for (int i = 0; i < model_count; i++) {
            if (strcmp(n64_model_paths[i], path) == 0) {
                return i;
            }
        }
        return -1;
    }
#endif

int load_gltf(const char* path, const char* name) {

#ifdef N64
    int existing_id = findModelByPath(path);
    if (existing_id != -1) {
        printf("Reusing existing model ID %d (%s)\n", existing_id, path);
        return existing_id;
    }
    if (model_count >= MAX_MODELS) {
        fprintf(stderr, "Model limit reached\n");
        return OBJ_MODEL_LIMIT_REACHED;
    }
    models[model_count] = model64_load(path);
    return model_count++;
#else

    if (model_count >= MAX_MODELS) {
        fprintf(stderr, "Model limit reached\n");
        return OBJ_MODEL_LIMIT_REACHED;
    }

    cgltf_options options = {0};
    cgltf_data* data = NULL;
    cgltf_result result = cgltf_parse_file(&options, path, &data);
    
    if (result != cgltf_result_success) {
        fprintf(stderr, "Failed to parse %s : error code %d\n", path, result);
        int error_code = -1;
        switch(result)
        {
            case cgltf_result_success: /* NOP */ break;
            case cgltf_result_data_too_short: printf("data too short\n"); error_code = OBJ_SHORT_DATA; break;
            case cgltf_result_unknown_format: printf("unknown format\n");  error_code = OBJ_UNKNOWN_FORMAT; break;
            case cgltf_result_invalid_json: printf("invalid json\n");  error_code = OBJ_INVALID_JSON; break;
            case cgltf_result_invalid_gltf: printf("invalid gltf\n");  error_code = OBJ_INVALID_GLTF;break;
            case cgltf_result_invalid_options: printf("invalid options\n");  error_code = OBJ_INVALID_OPTIONS;break;
            case cgltf_result_file_not_found: printf("file not found\n");  error_code = OBJ_FILE_NOT_FOUND;break;
            case cgltf_result_io_error: printf("io error\n");  error_code = OBJ_IO_ERROR;break;
            case cgltf_result_out_of_memory: printf("out of memory\n");  error_code = OBJ_OUT_OF_MEMORY;break;
            case cgltf_result_legacy_gltf: printf("legacy gltf\n");  error_code = OBJ_LEGACY_GLTF;break;
        }
        return error_code;
    }

    result = cgltf_load_buffers(&options, data, path);
    if (result != cgltf_result_success) {
        fprintf(stderr, "Failed to load buffers for %s\n", path);
        cgltf_free(data);
        return OBJ_LOAD_BUFFER_FAIL;
    }

    result = cgltf_validate(data);
    if (result != cgltf_result_success) {
        fprintf(stderr, "Invalid GLTF: %s\n", path);
        cgltf_free(data);
        return OBJ_GLTF_NOT_VALID;
    }

    // Store the loaded data
    models[model_count].data = data;
    strncpy(models[model_count].name, name, sizeof(models[model_count].name) - 1);
    models[model_count].name[sizeof(models[model_count].name) - 1] = '\0';
#endif

#ifdef N64
    strncpy(n64_model_paths[model_count], path, sizeof(n64_model_paths[0]) - 1);
    n64_model_paths[model_count][sizeof(n64_model_paths[0]) - 1] = '\0';
#endif

    return model_count++;
}

struct Mesh load_to_mesh(int model_index)
{
    struct Mesh ziz_mesh = Mesh_CreateEmpty();

    if (model_index < 0 || model_index >= model_count) {
        fprintf(stderr, "Invalid model index: %d\n", model_index);
        return ziz_mesh;
    }

    cgltf_data* data = models[model_index].data;
    if (!data) {
        fprintf(stderr, "Model %d has null data\n", model_index);
        return ziz_mesh;
    }

    // Create Mesh struct

    if (data->meshes_count > 1)
    {
        printf("Error more than one mesh in model index %d\n", model_index);
        return ziz_mesh;
    }

    size_t positions_bytes = 0;
    size_t normals_bytes = 0;
    size_t texcoords_bytes = 0;
    size_t indices_bytes = 0;

    for (cgltf_size i = 0; i < data->meshes_count; i++) {
        cgltf_mesh* mesh = &data->meshes[i];

        for (cgltf_size j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive* primitive = &mesh->primitives[j];

            // Get pointers to all attribute data
            const float* positions = NULL;
            const float* normals = NULL;
            const float* texcoords = NULL;
            const float* colors = NULL;
            int color_components = 0;

            for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
                cgltf_attribute* attr = &primitive->attributes[k];
                cgltf_accessor* accessor = attr->data;

                if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) {
                    continue;
                }

                // read data amount for allocation
                cgltf_size floats_in_component = cgltf_num_components(attr->type);
                cgltf_size item_count = cgltf_accessor_unpack_floats(accessor, NULL, floats_in_component);

                const float* data = (const float*)((const uint8_t*)accessor->buffer_view->buffer->data +
                accessor->buffer_view->offset +
                accessor->offset);

                switch (attr->type) {
                    case cgltf_attribute_type_position:
                        positions_bytes = sizeof(float) * item_count;
                        printf("Allocate %d bytes for positions\n", positions_bytes);
                        ziz_mesh.positions = (float*)AllocateGPUMemory(positions_bytes);
                        ziz_mesh.vertex_count = item_count / 3;
                        positions = data;
                        break;
                    case cgltf_attribute_type_normal:
                        normals_bytes = sizeof(float) * item_count;
                        printf("Allocate %d bytes for normals\n", normals_bytes);
                        ziz_mesh.normals = (float*)AllocateGPUMemory(normals_bytes);
                        normals = data;
                        break;
                    case cgltf_attribute_type_texcoord:
                        texcoords_bytes = sizeof(float) * item_count;
                        printf("Allocate %d bytes for tex coords\n", texcoords_bytes);
                        ziz_mesh.texcoords = (float*)AllocateGPUMemory(texcoords_bytes);
                        texcoords = data;
                        break;
                    case cgltf_attribute_type_color:
                        colors = data;
                        color_components = (accessor->type == cgltf_type_vec3) ? 3 : 4;
                        break;
                    default:
                        break;
                }
            }

            // Read elements
            if (primitive->indices) {
                cgltf_accessor* accessor = primitive->indices;
                if (!accessor->buffer_view || !accessor->buffer_view->buffer) {
                    continue;
                }

                GLenum type = GL_UNSIGNED_SHORT;
                if (accessor->component_type == cgltf_component_type_r_32u) {
                    type = GL_UNSIGNED_INT;
                    cgltf_size bytes_in_index = 4;
                    cgltf_size item_count = cgltf_accessor_unpack_indices(accessor, NULL, bytes_in_index, accessor->count);
                    indices_bytes = sizeof(unsigned int) * item_count;
                    ziz_mesh.indices = (unsigned short*)AllocateGPUMemory(indices_bytes);
                    printf("ERROR Bunny has Unsigned Int indices");
                }
                else
                {
                    cgltf_size bytes_in_index = 2;
                    cgltf_size item_count = cgltf_accessor_unpack_indices(accessor, NULL, bytes_in_index, accessor->count);
                    indices_bytes = sizeof(unsigned short) * item_count;
                    printf("Allocate %d bytes for indices\n", indices_bytes);
                    ziz_mesh.indices = (unsigned short*)AllocateGPUMemory(indices_bytes);
                    ziz_mesh.index_count = item_count;
                }


                const void* indices = (const uint8_t*)accessor->buffer_view->buffer->data +
                accessor->buffer_view->offset +
                accessor->offset;

                // Draw with indices
                // glBegin(GL_TRIANGLES);
                for (cgltf_size idx = 0; idx < accessor->count; idx++) {
                    unsigned int index;
                    if (type == GL_UNSIGNED_SHORT) {
                        index = ((const unsigned short*)indices)[idx];
                    } else {
                        index = ((const unsigned int*)indices)[idx];
                    }
                    ziz_mesh.indices[idx] = index;

                    if (normals) {
                        // glNormal3fv(&normals[index * 3]);
                        size_t i = index * 3;
                        ziz_mesh.normals[i + 0] = normals[i + 0];
                        ziz_mesh.normals[i + 1] = normals[i + 1];
                        ziz_mesh.normals[i + 2] = normals[i + 2];
                    }
                    if (texcoords) {
                        //glTexCoord2f(texcoords[index * 2 + 0], 1.0f - texcoords[index * 2 + 1]);
                        size_t i = index * 2;
                        ziz_mesh.texcoords[i + 0] = texcoords[i + 0];
                        ziz_mesh.texcoords[i + 1] = texcoords[i + 1]; // Flip y here if needed
                    }
                    if (colors) {
                        // glColor3f(colors[index * 3 + 0], colors[index * 3 + 1], colors[index * 3 + 2]);
                    }
                    //glVertex3fv(&positions[index * 3]);
                    size_t i = index * 3;
                    ziz_mesh.positions[i + 0] = positions[i + 0];
                    ziz_mesh.positions[i + 1] = positions[i + 1];
                    ziz_mesh.positions[i + 2] = positions[i + 2];
                }
                // glEnd();
            } else {
                // Draw without indices
                if (positions) {
                    cgltf_size vertex_count = primitive->attributes[0].data->count;
                    //glBegin(GL_TRIANGLES);
                    for (cgltf_size v = 0; v < vertex_count; v++) {
                        if (normals) {
                            // glNormal3fv(&normals[v * 3]);
                            size_t i = v * 3;
                            ziz_mesh.normals[i + 0] = normals[i + 0];
                            ziz_mesh.normals[i + 1] = normals[i + 1];
                            ziz_mesh.normals[i + 2] = normals[i + 2];
                        }
                        if (texcoords) {
                            // glTexCoord2f(texcoords[v * 2 + 0], 1.0f - texcoords[v * 2 + 1]);
                            size_t i = v * 2;
                            ziz_mesh.texcoords[i + 0] = texcoords[i + 0];
                            ziz_mesh.texcoords[i + 1] = texcoords[i + 1];
                        }
                        /*
                         i f (colors) {                   *
                         if (color_components == 3) {
                             glColor3fv(&colors[v * 3]);
                    } else {
                        glColor4fv(&colors[v * 4]);
                    }
                    }
                    */
                        //glVertex3fv(&positions[v * 3]);
                        size_t i = v * 3;
                        ziz_mesh.positions[i + 0] = positions[i + 0];
                        ziz_mesh.positions[i + 1] = positions[i + 1];
                        ziz_mesh.positions[i + 2] = positions[i + 2];
                    }
                    // glEnd();
                }
            }
        }
    }
    if (positions_bytes > 0)
    {
        FlushGPUCache(ziz_mesh.positions, positions_bytes );
    }
    if (normals_bytes > 0)
    {
        FlushGPUCache(ziz_mesh.normals, normals_bytes );
    }
    if (texcoords_bytes > 0)
    {
        FlushGPUCache(ziz_mesh.texcoords, texcoords_bytes );
    }
    return ziz_mesh;
}

void draw_gltf(int model_index) {

#ifdef N64
    int mesh_count = model64_get_mesh_count(models[model_index]);
    for(int mesh_index = 0; mesh_index < mesh_count; mesh_index++) {
        model64_draw_mesh(model64_get_mesh(models[model_index], mesh_index));
    }
#else

    if (model_index < 0 || model_index >= model_count) {
        fprintf(stderr, "Invalid model index: %d\n", model_index);
        return;
    }

    cgltf_data* data = models[model_index].data;
    if (!data) {
        fprintf(stderr, "Model %d has null data\n", model_index);
        return;
    }

    for (cgltf_size i = 0; i < data->meshes_count; i++) {
        cgltf_mesh* mesh = &data->meshes[i];
        
        for (cgltf_size j = 0; j < mesh->primitives_count; j++) {
            cgltf_primitive* primitive = &mesh->primitives[j];
            
            // Get pointers to all attribute data
            const float* positions = NULL;
            const float* normals = NULL;
            const float* texcoords = NULL;
            const float* colors = NULL;
            int color_components = 0;
            
            for (cgltf_size k = 0; k < primitive->attributes_count; k++) {
                cgltf_attribute* attr = &primitive->attributes[k];
                cgltf_accessor* accessor = attr->data;
                
                if (!accessor || !accessor->buffer_view || !accessor->buffer_view->buffer) {
                    continue;
                }

                const float* data = (const float*)((const uint8_t*)accessor->buffer_view->buffer->data + 
                                                accessor->buffer_view->offset + 
                                                accessor->offset);

                switch (attr->type) {
                    case cgltf_attribute_type_position:
                        positions = data;
                        break;
                    case cgltf_attribute_type_normal:
                        normals = data;
                        break;
                    case cgltf_attribute_type_texcoord:
                        texcoords = data;
                        break;
                    case cgltf_attribute_type_color:
                        colors = data;
                        color_components = (accessor->type == cgltf_type_vec3) ? 3 : 4;
                        break;
                    default:
                        break;
                }
            }

            // Draw elements
            if (primitive->indices) {
                cgltf_accessor* accessor = primitive->indices;
                if (!accessor->buffer_view || !accessor->buffer_view->buffer) {
                    continue;
                }

                const void* indices = (const uint8_t*)accessor->buffer_view->buffer->data + 
                                    accessor->buffer_view->offset + 
                                    accessor->offset;

                GLenum type = GL_UNSIGNED_SHORT;
                if (accessor->component_type == cgltf_component_type_r_32u) {
                    type = GL_UNSIGNED_INT;
                }

                // Draw with indices
                glBegin(GL_POINTS);
                for (cgltf_size idx = 0; idx < accessor->count; idx++) {
                    unsigned int index;
                    if (type == GL_UNSIGNED_SHORT) {
                        index = ((const unsigned short*)indices)[idx];
                    } else {
                        index = ((const unsigned int*)indices)[idx];
                    }

                    if (normals) {
                        glNormal3fv(&normals[index * 3]);
                    }
                    if (texcoords) {
                        glTexCoord2f(texcoords[index * 2 + 0], 1.0f - texcoords[index * 2 + 1]);
                    }
                    if (colors) {
                        glColor3f(colors[index * 3 + 0], colors[index * 3 + 1], colors[index * 3 + 2]);
                    }
                    glVertex3fv(&positions[index * 3]);
                }
                glEnd();
            } else {
                // Draw without indices
                if (positions) {
                    cgltf_size vertex_count = primitive->attributes[0].data->count;
                    glBegin(GL_TRIANGLES);
                    for (cgltf_size v = 0; v < vertex_count; v++) {
                        if (normals) {
                            glNormal3fv(&normals[v * 3]);
                        }
                        if (texcoords) {
                            glTexCoord2f(texcoords[v * 2 + 0], 1.0f - texcoords[v * 2 + 1]);
                        }
                        if (colors) {
                            if (color_components == 3) {
                                glColor3fv(&colors[v * 3]);
                            } else {
                                glColor4fv(&colors[v * 4]);
                            }
                        }
                        glVertex3fv(&positions[v * 3]);
                    }
                    glEnd();
                }
            }
        }
    }
#endif
}

void cleanup_gltf() {
#ifndef N64
    for (int i = 0; i < model_count; i++) {
        if (models[i].data) {
            cgltf_free(models[i].data);
        }
    }
    model_count = 0;
#endif
}

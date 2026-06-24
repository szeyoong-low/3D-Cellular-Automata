#ifndef SHADER_H
#define SHADER_H

#include <cglm/types.h>
#include <glad/glad.h>
#include <stdbool.h>
#include <sys/types.h>

#define SHADER_PATH(name) ("shaders/" name ".glsl")
#define VERTEX_SHADER SHADER_PATH("vertex")
#define FRAGMENT_SHADER SHADER_PATH("fragment")
#define HIDDEN_CELL_COMPUTE_SHADER SHADER_PATH("hidden")
#define FRUSTUM_COMPUTE_SHADER SHADER_PATH("frustum")
#define BITONIC_SORT_COMPUTE_SHADER SHADER_PATH("sort")

#define VIEW_PROJ_UNIFORM "uViewProj"
#define LIGHT_POS_UNIFORM "uLightPos"
#define CAMERA_POS_UNIFORM "uCameraPos"
#define LIGHTING_UNIFORM "uLighting"
#define WIDTH_UNIFORM "uWidth"
#define HEIGHT_UNIFORM "uHeight"
#define DEPTH_UNIFORM "uDepth"
#define SORT_BLOCK_UNIFORM "uBlockSize"
#define SORT_STEP_UNIFORM "uStepSize"

#define NUM_WORKERS(dimension, local_size)                                     \
  ((uint)ceil((float)dimension / (float)local_size))

#define CULLING_LOCAL_SIZE_X 10
#define CULLING_LOCAL_SIZE_Y 10
#define CULLING_LOCAL_SIZE_Z 10

#define DISPATCH_CULLING_COMPUTE(width, height, depth)                         \
  glDispatchCompute(NUM_WORKERS(width, CULLING_LOCAL_SIZE_X),                  \
                    NUM_WORKERS(height, CULLING_LOCAL_SIZE_Y),                 \
                    NUM_WORKERS(depth, CULLING_LOCAL_SIZE_Z));

#define SORTING_LOCAL_SIZE 1024
#define SORTING_LOCAL_MAX_STEP (SORTING_LOCAL_SIZE / 2)
#define SORTING_NUM_WORKERS_Y 1
#define SORTING_NUM_WORKERS_Z 1

typedef struct {
  GLenum type; // GL_VERTEX_SHADER, GL_FRAGMENT_SHADER, etc.
  const char *path;
} ShaderDef;

// In OpenGL, a program is a compiled and linked set of shaders (vertex and
// fragment for rendering pipelines) that the GPU executes for every draw call.
// Loads, compiles, and links all shaders into one program.
// Returns a programme on success and 0 on failure.
extern GLuint shader_build_program(const ShaderDef *shaders, int count);

// Note: the following functions make shader_prog the currently bound programme

// Upload width, height, and depth uniform variables to the provided shader
// programme. The variable names MUST match the respective macros above.
// The programme must be a valid shader programme.
extern void shader_upload_dim_uniforms(GLuint shader_prog, uint width,
                                       uint height, uint depth);

extern void shader_upload_lighting_uniforms(GLuint shader_prog, bool lighting,
                                            vec3 light_pos);

#endif

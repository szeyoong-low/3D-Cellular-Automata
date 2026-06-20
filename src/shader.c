#include "shader.h"
#include <stdio.h>
#include <stdlib.h>

#define READ_FAIL(file, msg, path)                                             \
  fprintf(stderr, "%s at %s\n", msg, path);                                    \
  fclose(file);                                                                \
  return NULL;

// Length of the buffer used to retrieve shader/program compiler error messages
#define LOG_SIZE 512
#define FILE_START_OFFSET 0
#define NULL_BYTE '\0'
#define RETURN_FAIL 0

// Reads a shader source file into a heap-allocated null-terminated string.
// Caller must free the returned pointer. Returns NULL on failure.
static char *shader_load(const char *path);

// Compiles a single shader stage.
// Returns 0 on failure (error printed to stderr).
static GLuint shader_compile(GLenum type, const char *src);

GLuint shader_build_program(const ShaderDef *shaders, int count) {
  const GLuint program = glCreateProgram();

  // Track compiled shader handles so they can be deleted after linking or on
  // failure — shader objects are only needed until glLinkProgram bakes them in
  GLuint *compiled_shaders = calloc((size_t)count, sizeof(GLuint));
  if (compiled_shaders == NULL) {
    glDeleteProgram(program);
    return RETURN_FAIL;
  }

  int compiled_count = 0;

  for (int i = 0; i < count; i++) {
    char *src = shader_load(shaders[i].path);
    if (src == NULL) {
      goto fail;
    }

    const GLuint shader = shader_compile(shaders[i].type, src);
    free(src);
    if (shader == 0) {
      goto fail;
    }

    glAttachShader(program, shader);
    compiled_shaders[compiled_count++] = shader;
    continue;

  fail:
    for (int j = 0; j < compiled_count; j++) {
      glDeleteShader(compiled_shaders[j]);
    }
    free(compiled_shaders);
    glDeleteProgram(program);
    return RETURN_FAIL;
  }

  glLinkProgram(program);

  for (int i = 0; i < compiled_count; i++) {
    glDeleteShader(compiled_shaders[i]);
  }
  free(compiled_shaders);

  GLint success;
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char log[LOG_SIZE];
    glGetProgramInfoLog(program, LOG_SIZE, NULL, log);
    fprintf(stderr, "Program link error:\n%s\n", log);
    glDeleteProgram(program);
    return RETURN_FAIL;
  }

  return program;
}

inline void shader_upload_dim_uniforms(GLuint shader_prog, uint width,
                                       uint height, uint depth) {
  const GLint width_loc = glGetUniformLocation(shader_prog, WIDTH_UNIFORM);
  const GLint height_loc = glGetUniformLocation(shader_prog, HEIGHT_UNIFORM);
  const GLint depth_loc = glGetUniformLocation(shader_prog, DEPTH_UNIFORM);

  glUseProgram(shader_prog);
  glUniform1ui(width_loc, width);
  glUniform1ui(height_loc, height);
  glUniform1ui(depth_loc, depth);
}

inline void shader_upload_lighting_uniforms(GLuint shader_prog,
                                            bool lighting_flag,
                                            vec3 light_pos) {
  const GLint light_pos_loc =
      glGetUniformLocation(shader_prog, LIGHT_POS_UNIFORM);
  const GLint lighting_loc =
      glGetUniformLocation(shader_prog, LIGHTING_UNIFORM);

  glUseProgram(shader_prog);
  glUniform1i(lighting_loc, lighting_flag);
  glUniform3fv(light_pos_loc, 1, light_pos);
}

char *shader_load(const char *path) {
  FILE *shader_file = fopen(path, "r");

  if (shader_file == NULL) {
    fprintf(stderr, "Failed to read from shader file %s\n", path);
    return NULL;
  }

  if (fseek(shader_file, FILE_START_OFFSET, SEEK_END) != 0) {
    READ_FAIL(shader_file, "Failed to read from shader file", path)
  }

  const long size = ftell(shader_file);

  if (size < 0) {
    READ_FAIL(shader_file, "Failed to read from shader file", path)
  }

  if (fseek(shader_file, FILE_START_OFFSET, SEEK_SET) != 0) {
    READ_FAIL(shader_file, "Failed to read from shader file", path)
  }

  char *src = malloc((size_t)size + 1);
  if (src == NULL) {
    READ_FAIL(shader_file, "Heap overflow: unable to load shader file", path)
  }

  size_t read = fread(src, sizeof(char), (size_t)size, shader_file);
  // Text mode may convert \r\n → \n, so bytes read can be less than size.
  // Use the actual count returned by fread to place the null terminator.
  src[read] = NULL_BYTE;

  if (fclose(shader_file) != 0) {
    fprintf(stderr, "Failed to close shader file");
    return NULL;
  }
  return src;
}

// All GPU objects in OpenGL (e.g. VAOs, VBOs, shaders, programs, textures) are
// represented as GLuint handles. The actual data lives inside the GPU driver,
// not in your C process. Your C code can't hold a pointer to GPU memory
// directly (the address spaces are separate). Instead, OpenGL maintains a table
// of objects on the driver side, and gives you back an integer ID as a key into
// that table. 0 is the NULL handle.

GLuint shader_compile(GLenum type, const char *src) {
  GLuint shader = glCreateShader(type);
  glShaderSource(shader, 1, &src, NULL); // NULL: source is null-terminated
  glCompileShader(shader);

  GLint success;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
  if (success == 0) {
    char log[LOG_SIZE];
    glGetShaderInfoLog(shader, LOG_SIZE, NULL, log);
    fprintf(stderr, "Shader compile error:\n%s\n", log);
    glDeleteShader(shader);
    return RETURN_FAIL;
  }

  return shader;
}
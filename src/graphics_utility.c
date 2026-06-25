#include "graphics_utility.h"
#include "mesh.h"
#include "shader.h"
#include <stdio.h>

#define NUM_MIPS 1 // Only 1 mip level (resolution)
#define MIPS_LEVEL 0

const DrawElementsIndirectCommand draw_indirect_cmd = {
    .count = CUBE_VERTEX_COUNT, // # indices/instance
    .instanceCount = 0, // # instances/draw (updated by the compute shader)
    .firstIndex = 0,    // Starting offset into EBO
    .baseVertex = 0,    // Added to each index value before fetching from VBO
    .baseInstance =
        0, // Starting instance ID (offset into per-isntance attributes)
};

inline void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
  WindowUserPointer *window_user_pointer = glfwGetWindowUserPointer(window);
  *window_user_pointer->fb_width = width;
  *window_user_pointer->fb_height = height;
  glViewport(0, 0, width, height);

  if (!window_user_pointer->opacity || width == 0 || height == 0) {
    return;
  }

  GLuint blending_framebuffer = window_user_pointer->blending_framebuffer;
  GLuint *accum_texture = window_user_pointer->accum_texture;
  GLuint *reveal_texture = window_user_pointer->reveal_texture;

  glDeleteTextures(1, accum_texture);
  glDeleteTextures(1, reveal_texture);
  build_framebuffer(blending_framebuffer, accum_texture, reveal_texture, width,
                    height);
}

inline void build_framebuffer(GLuint framebuffer, GLuint *accum_texture,
                              GLuint *reveal_texture, int width, int height) {
  glCreateTextures(GL_TEXTURE_2D, 1, accum_texture);
  glTextureStorage2D(*accum_texture, NUM_MIPS, GL_RGBA16F, width, height);
  glNamedFramebufferTexture(framebuffer, ACCUM_COLOR_ATTACHMENT, *accum_texture,
                            MIPS_LEVEL);

  glCreateTextures(GL_TEXTURE_2D, 1, reveal_texture);
  glTextureStorage2D(*reveal_texture, NUM_MIPS, GL_R8, width, height);
  glNamedFramebufferTexture(framebuffer, REVEAL_COLOR_ATTACHMENT,
                            *reveal_texture, MIPS_LEVEL);

  // Specify buffers into which fragment colors or data values will be written
  glNamedFramebufferDrawBuffers(
      framebuffer, 2, (GLenum[]){GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1});
}

inline void error_callback(int error, const char *description) {
  fprintf(stderr, "GLFW error %d: %s\n", error, description);
}

// Credit:
// https://graphics.stanford.edu/%7Eseander/bithacks.html#RoundUpPowerOf2
inline ulong next_power_two(ulong n) {
  n--;
  n |= n >> 1;
  n |= n >> 2;
  n |= n >> 4;
  n |= n >> 8;
  n |= n >> 16;
  n |= n >> 32;
  return n + 1;
}
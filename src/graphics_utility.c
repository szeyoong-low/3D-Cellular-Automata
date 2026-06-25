#include "graphics_utility.h"
#include "mesh.h"
#include <stdio.h>

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

  if (width == 0 || height == 0) {
    return;
  }

  GLuint blending_framebuffer = window_user_pointer->framebuffer;
  GLuint *colour_texture = window_user_pointer->colour_texture;

  glDeleteTextures(1, colour_texture);
  build_framebuffer(blending_framebuffer, colour_texture, width, height);
}

inline void build_framebuffer(GLuint framebuffer, GLuint *colour_texture,
                              int width, int height) {
  glCreateTextures(GL_TEXTURE_2D, 1, colour_texture);
  // Only 1 mip level (resolution)
  glTextureStorage2D(*colour_texture, 1, GL_RGBA8, width, height);
  // Attach the texture to the framebuffer object at location 0
  glNamedFramebufferTexture(framebuffer, BLENDING_FRAMEBUFFER_BINDING,
                            *colour_texture, 0);
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
#include "graphics_utility.h"
#include "face.h"
#include <stdio.h>

#define MASK_4_BITS 0xF

const DrawElementsIndirectCommand draw_indirect_cmd = {
    .count = FACE_TOTAL_VERTICES, // # indices/instance
    .instanceCount = 0, // # instances/draw (updated by the compute shader)
    .firstIndex = 0,    // Starting offset into EBO
    .baseVertex = 0,    // Added to each index value before fetching from VBO
    .baseInstance =
        0, // Starting instance ID (offset into per-isntance attributes)
};

inline void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height) {
  (void)window; // parameter required by GLFW signature but unused here
  glViewport(0, 0, width, height);
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

inline int next_multiple_16(int n) {
  return (n + MASK_4_BITS) & ~MASK_4_BITS;
}
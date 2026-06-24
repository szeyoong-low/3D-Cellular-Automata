#ifndef GRAPHICS_UTILITY_H
#define GRAPHICS_UTILITY_H

#include <GLFW/glfw3.h>
#include <sys/types.h>

#define WINDOW_WIDTH 800  // Fallback only
#define WINDOW_HEIGHT 600 // Fallback only

#define POWER_TWO(exp) ((uint)pow(2.0L, (double)exp))

typedef struct {
  uint count;
  uint instanceCount;
  uint firstIndex;
  int baseVertex;
  uint baseInstance;
} DrawElementsIndirectCommand;

extern const DrawElementsIndirectCommand draw_indirect_cmd;

extern void error_callback(int error, const char *description);

// Keeps the GL viewport matched to the framebuffer when the window is resized
extern void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height);

extern ulong next_power_two(ulong n);

#endif
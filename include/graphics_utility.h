#ifndef GRAPHICS_UTILITY_H
#define GRAPHICS_UTILITY_H

#include <GLFW/glfw3.h>
#include <sys/types.h>

#define WINDOW_WIDTH 800  // Fallback only
#define WINDOW_HEIGHT 600 // Fallback only

#define CULLING_LOCAL_SIZE_X 10.0F
#define CULLING_LOCAL_SIZE_Y 10.0F
#define CULLING_LOCAL_SIZE_Z 10.0F
#define SORTING_LOCAL_SIZE_X 256.0F
#define SORTING_NUM_WORKERS_Y 1
#define SORTING_NUM_WORKERS_Z 1
#define NUM_WORKERS(dimension, local_size)                                     \
  ((uint)ceil((float)dimension / local_size))

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
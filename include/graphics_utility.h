#ifndef GRAPHICS_UTILITY_H
#define GRAPHICS_UTILITY_H

#include "camera.h"
#include <GLFW/glfw3.h>
#include <sys/types.h>

#define DESTROY_AND_EXIT(window, success)                                      \
  glfwDestroyWindow(window);                                                   \
  glfwTerminate();                                                             \
  return (success) ? EXIT_SUCCESS : EXIT_FAILURE;

#define TITLE_BUFFER_SIZE 64
#define FPS_UPDATE_INTERVAL 0.4

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

typedef struct {
  Camera *camera;
  int *fb_width;
  int *fb_height;
} WindowUserPointer;

extern const DrawElementsIndirectCommand draw_indirect_cmd;

extern void error_callback(int error, const char *description);

// Keeps the GL viewport matched to the framebuffer when the window is resized
extern void framebuffer_size_callback(GLFWwindow *window, int width,
                                      int height);

extern ulong next_power_two(ulong n);

#endif
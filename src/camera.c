#include "camera.h"
#include "graphics_utility.h"

#define DRAG_SENSITIVITY 0.005F // radians of rotation per pixel dragged
#define ZOOM_SENSITIVITY 0.1F   // world units per scroll tick
// Exponential zoom: proportional to current radius
#define ZOOM_PCT_CHANGE(yoffset) (1.0F - (float)yoffset * ZOOM_SENSITIVITY)
#define RADIUS_MIN 1.0F  // closest the camera can get to the target
#define ELEV_MAX 1.3963F // 80 degrees
#define FIELD_OF_VIEW (float)GLM_PI_4 // 45 degrees

#define AZIMUTH_INIT 0.5F   // 29 degrees off-axis so depth is visible
#define ELEVATION_INIT 0.3F // 17 degrees upward tilt to see the top face
#define LAST_X_INIT 0.0F
#define LAST_Y_INIT 0.0F

#define SQUARE(n) (n * n)

// Callbacks to update the Camera object when events occur in the window

// Sets dragging on left-click and reads current cursor position into
// last_x/last_y at the moment of press
static void on_mouse_button(GLFWwindow *window, int button, int action,
                            int mods);

// If the cursor is being dragged, compute dx/dy from the last position,
// updates azimuth and elevation
static void on_cursor_pos(GLFWwindow *window, double x, double y);

// Shrinks/grows radius
static void on_scroll(GLFWwindow *window, double xoffset, double yoffset);

vec3 origin_coords = {0.0F, 0.0F, 0.0F};

vec3 up_direction = {0.0F, 1.0F, 0.0F};

inline float camera_grid_bounding_radius(uint width, uint height, uint depth) {
  return sqrtf((float)(SQUARE(width) + SQUARE(height) + SQUARE(depth))) / 2.0F;
}

void camera_init(Camera *camera, GLFWwindow *window, float initial_radius) {
  camera->radius = initial_radius;
  camera->azimuth = AZIMUTH_INIT;
  camera->elevation = ELEVATION_INIT;
  camera->dragging = false;
  camera->last_x = LAST_X_INIT;
  camera->last_y = LAST_Y_INIT;

  glfwSetMouseButtonCallback(window, on_mouse_button);
  glfwSetCursorPosCallback(window, on_cursor_pos);
  glfwSetScrollCallback(window, on_scroll);
}

// The camera has spherical coordinates as they correspond naturally with the
// updates made by the callbacks (mouse drag changes azimuth/elevation, scroll
// changes radius)
void camera_position(Camera *camera, vec3 out) {
  const float cos_elev = cosf(camera->elevation);
  out[0] = camera->radius * cos_elev * sinf(camera->azimuth); // x
  out[1] = camera->radius * sinf(camera->elevation);          // y
  out[2] = camera->radius * cos_elev * cosf(camera->azimuth); // z
}

void camera_update_proj(GLFWwindow *window, float bounding_radius,
                        float camera_radius, mat4 proj) {
  // The projection matrix gives vertices a coordinate w that represents the
  // positive depth along the camera's view axis. The normalised device
  // coordinates are then the xyz coordinates divided by w, so that objects
  // further from the camera are scaled down more (perspective divide).

  // Use framebuffer size (# physical pixels) instead of window size
  // (# logical pixels) as HiDPI displays have more physical than logical pixels
  int fb_width, fb_height;
  glfwGetFramebufferSize(window, &fb_width, &fb_height);
  glViewport(0, 0, fb_width, fb_height);

  const float aspect = (float)fb_width / (float)fb_height;
  // Far plane covers the back of the grid plus a 2× margin for scrolling out
  const float far_plane = camera_radius + bounding_radius * 2.0F;
  // near = 0.1 (clips geometry very close to the camera), far clips everything
  // further away
  glm_perspective(FIELD_OF_VIEW, aspect, NEAR_PLANE, far_plane, proj);
}

void on_mouse_button(GLFWwindow *window, int button, int action, int mods) {
  (void)mods;

  if (button != GLFW_MOUSE_BUTTON_LEFT) {
    return;
  }

  Camera *camera =
      ((WindowUserPointer *)glfwGetWindowUserPointer(window))->camera;

  if (action == GLFW_PRESS) {
    camera->dragging = true;
    glfwGetCursorPos(window, &camera->last_x, &camera->last_y);
  } else {
    camera->dragging = false;
  }
}

// GLFW reports screen coordinates (2D) with the top-left corner as origin.
// Don't confuse this with the coordinates of the world space (3D)
// In the world coordinates, y is up (following graphics convention) and z is
// the direction facing away from you

void on_cursor_pos(GLFWwindow *window, double x, double y) {
  Camera *camera =
      ((WindowUserPointer *)glfwGetWindowUserPointer(window))->camera;

  if (!camera->dragging) {
    return;
  }

  float dx = (float)(x - camera->last_x);
  float dy = (float)(y - camera->last_y);
  camera->last_x = x;
  camera->last_y = y;

  // Dragging right causes camera to swing counterclockwise around y-axis.
  // The screen rotates left.
  // It is important to note that the camera orbits around a fixed point instead
  // of spinning in place, so the direction the camera travels and the direction
  // the scene appears to rotate are always opposite (relative motion)
  camera->azimuth += dx * DRAG_SENSITIVITY;
  // Screen y increases downward, so dragging down must lower elevation
  camera->elevation -= dy * DRAG_SENSITIVITY;

  // Leaves headroom before the poles where glm_lookat can flip if the
  // up-vector aligns with the view direction.
  if (camera->elevation > ELEV_MAX) {
    camera->elevation = ELEV_MAX;
  }

  if (camera->elevation < -ELEV_MAX) {
    camera->elevation = -ELEV_MAX;
  }
}

void on_scroll(GLFWwindow *window, double xoffset, double yoffset) {
  (void)xoffset; // Only care about vertical scroll

  Camera *camera =
      ((WindowUserPointer *)glfwGetWindowUserPointer(window))->camera;
  camera->radius *= ZOOM_PCT_CHANGE(yoffset);

  // Floored so that you can't zoom inside the grid
  if (camera->radius < RADIUS_MIN) {
    camera->radius = RADIUS_MIN;
  }
}

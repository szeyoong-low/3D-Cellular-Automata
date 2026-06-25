#ifndef CAMERA_H
#define CAMERA_H

#include <glad/glad.h>
#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>
#include <cglm/cglm.h>
#include <cglm/types.h>

#define NEAR_PLANE 0.1F

// Orbit camera stored in spherical coordinates (graphics convention: y is up)
// - azimuth: horizontal angle in the xz-plane around the y-axis (radians)
// - elevation: vertical angle above the xz-plane (radians), clamped ±80 degrees
//              so that the camera does not flip
// - radius: distance from the target point
typedef struct {
  float radius; // World coordinates (3D)
  float azimuth;
  float elevation;

  // Mouse drag state updated by GLFW callbacks
  bool dragging;
  double last_x; // Screen coordinates (2D)
  double last_y;
} Camera;

extern vec3 origin_coords;
extern vec3 up_direction;

// Returns the radius of the smallest sphere that encloses a grid of
// width × height × depth cells measured from the grid centre.
// Use this to derive a sensible initial camera radius and projection far plane.
extern float camera_grid_bounding_radius(uint width, uint height, uint depth);

// Fill the camera with sensible defaults so it orbits the origin from a nice
// angle, and registers event callbacks for mouse clicks, cursor movements, and
// scroll.
// Caller MUST add the camera to the window user pointer.
extern void camera_init(Camera *camera, GLFWwindow *window,
                        float initial_radius);

// Convert spherical coordinates to a world-space eye (Cartesian coordinates).
// Pass the result directly to glm_lookat as the first argument.
extern void camera_position(Camera *camera, vec3 out);

// Side effects: updates window viewport size, writes new projection matrix into
//               proj, sets fb_width and fb_height
extern void camera_update_proj(int fb_width, int fb_height,
                               float bounding_radius, float camera_radius,
                               mat4 proj);

#endif
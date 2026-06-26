#include "args.h"
#include "cell_loader.h"
#include "simulate.h"
#include <dlfcn.h>
#include <glad/glad.h>
#include <sys/types.h>
#include <time.h>
// glad must precede GLFW as it defines GL types first
// The OpenGL driver (Nvidia/AMD/Mesa) ships on your machine, and your program
// has to look up every GL function at runtime by asking the OS for a function
// pointer by name as they're not linked at compile time.
#include <GLFW/glfw3.h>
// GLFW is a simple API on top of OpenGL for creating windows, contexts and
// surfaces, receiving input and events.
// Must come after glad is included so that types are defined
#include "buffer.h"
#include "camera.h"
#include "face.h"
#include "frustum.h"
#include "graphics_utility.h"
#include "shader.h"

int main(int argc, char **argv) {
  args args = parse_args(argc, argv);

  uint initial_seed = args.custom_seed ? args.seed : (uint)time(NULL);
  printf("Initial seed used: %d\n", initial_seed);
  srand(initial_seed);

  void *handle;
  CellConfig cell_config = load_cell_config(args.cell_path, &handle);
  const uint sim_width = args.width;
  const uint sim_width_padded = (uint)next_power_two(sim_width);
  const uint sim_height = args.height;
  const uint sim_height_padded = (uint)next_power_two(sim_height);
  const uint sim_depth = args.depth;
  const uint sim_depth_padded = (uint)next_power_two(sim_depth);
  const size_t sim_size = sim_width * sim_height * sim_depth;
  const size_t num_instances = sim_size * FACES_PER_CUBE;
  const double step_time = args.steptime;
  const bool opacity = args.opacity;

  Sim sim = sim_create(sim_width, sim_height, sim_depth, args.num_threads,
                       cell_config);

  // Register before glfwInit so errors during init are also reported
  glfwSetErrorCallback(error_callback);

  if (!glfwInit()) {
    return EXIT_FAILURE;
  }

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_MAXIMIZED, GLFW_TRUE);

  // A window is the OS-managed rectangle on screen with a title bar.
  // GLFW asks the operating system to create one. It owns the pixels on screen
  // (the framebuffer) and the event queue (mouse moves, key presses, close
  // button clicks). It works like the FILE * handler.
  GLFWwindow *window =
      glfwCreateWindow(WINDOW_WIDTH, WINDOW_HEIGHT, args.cell_path, NULL, NULL);
  if (!window) {
    glfwTerminate();
    return EXIT_FAILURE;
  }

  // An OpenGL context is maintained by the GPU driver to hold:
  // - Which buffers, textures, and shaders are currently bound
  // - What the current viewport dimensions are
  // - The depth test settings, blend modes, etc.

  // The context lives attached to a window, which provides the surface
  // it draws onto, but they're created separately (GLFW does it in one
  // call for convenience).

  // A context must be made "current" on a thread before any GL calls work.
  // This updates the driver's per-thread pointer to the "current" context for
  // the calling thread.
  glfwMakeContextCurrent(window);

  // glad is a generated function loader that loads function pointers specific
  // to the driver that owns the current context.
  if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress)) {
    DESTROY_AND_EXIT(window, false)
  }

  glEnable(GL_CULL_FACE);
  glCullFace(GL_BACK);

  mat4 view, proj, view_proj;
  GLuint zero = 0;
  vec3 eye;
  vec3 light_pos = {10.0F, 10.0F, 10.0F};
  int fb_width, fb_height;

  // For weighted-blended order-independent transparency
  GLuint blending_framebuffer = 0;
  GLuint accum_texture, reveal_texture;
  GLuint post_processing = 0;

  if (opacity) {
    glEnable(GL_BLEND);
    glCreateFramebuffers(1, &blending_framebuffer);
    glfwGetFramebufferSize(window, &fb_width, &fb_height);
    build_framebuffer(blending_framebuffer, &accum_texture, &reveal_texture,
                      fb_width, fb_height);

    post_processing = shader_build_program(
        (ShaderDef[]){{GL_VERTEX_SHADER, POST_PROCESS_VERTEX_SHADER},
                      {GL_FRAGMENT_SHADER, POST_PROCESS_FRAGMENT_SHADER}},
        2);

    shader_upload_integer(post_processing, ACCUM_TEXTURE_UNIFORM,
                          ACCUM_BINDING_TARGET);

    shader_upload_integer(post_processing, REVEAL_TEXTURE_UNIFORM,
                          REVEAL_BINDING_TARGET);
  } else {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
  }

  // Radius of the smallest sphere that encloses the grid, from its centre.
  // Used to size the initial camera distance, FOV, and far clipping plane.
  const float bounding_radius =
      camera_grid_bounding_radius(sim_width, sim_height, sim_depth);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Start 2× the bounding radius away so the full grid fits in the FOV
  Camera camera;
  camera_init(&camera, window, 2.0F * bounding_radius);

  // Allow callbacks to access state
  WindowUserPointer window_user_pointer = {
      .camera = &camera,
      .fb_width = &fb_width,
      .fb_height = &fb_height,
      .accum_texture = &accum_texture,
      .reveal_texture = &reveal_texture,
      .blending_framebuffer = blending_framebuffer,
      .opacity = opacity,
  };
  glfwSetWindowUserPointer(window, &window_user_pointer);

  // Shaders are loaded from disk relative to the working directory.
  // Run the binary from the project root: ./bin/cellular_automata
  const GLuint render_pipeline =
      shader_build_program((ShaderDef[]){{GL_VERTEX_SHADER, VERTEX_SHADER},
                                         {GL_FRAGMENT_SHADER, FRAGMENT_SHADER}},
                           2);

  const GLuint occlusion_culling = shader_build_program(
      (ShaderDef[]){{GL_COMPUTE_SHADER, OCCLUSION_COMPUTE_SHADER}}, 1);

  const GLuint frustum_culling = shader_build_program(
      (ShaderDef[]){{GL_COMPUTE_SHADER, FRUSTUM_COMPUTE_SHADER}}, 1);

  frustum_init(frustum_culling, opacity);

  // Cache uniform locations so that unnecessary driver round-trips are not done
  const GLint view_proj_loc =
      glGetUniformLocation(render_pipeline, VIEW_PROJ_UNIFORM);
  const GLint camera_pos_loc =
      glGetUniformLocation(render_pipeline, CAMERA_POS_UNIFORM);

  // Upload these once as they don't change per frame
  shader_upload_lighting_uniforms(render_pipeline, args.lighting, light_pos);

  shader_upload_integer(render_pipeline, OPACITY_UNIFORM, opacity);

  shader_upload_dim_uniforms(occlusion_culling, sim_width, sim_height,
                             sim_depth);

  shader_upload_integer(occlusion_culling, NO_WALLS_UNIFORM, !opacity || args.no_walls);

  shader_upload_dim_uniforms(frustum_culling, sim_width, sim_height, sim_depth);

  // VRAM buffer initialisation
  GLuint attribute_buffer, vertex_buffer, element_buffer, render_info_buffer,
      occlusion_buffer, instance_buffer, draw_indirect_buffer;

  attribute_buffer_init(&attribute_buffer);
  vertex_buffer_init(&vertex_buffer);
  element_buffer_init(&element_buffer);
  render_info_buffer_init(&render_info_buffer, sim_size, sim_render_info(sim));
  occlusion_buffer_init(&occlusion_buffer, sim_size);
  instance_buffer_init(&instance_buffer, num_instances);
  draw_indirect_buffer_init(&draw_indirect_buffer);

  double last_step_time = glfwGetTime(); // seconds as a double since glfwInit()
  glUseProgram(occlusion_culling);
  DISPATCH_CULLING_COMPUTE(sim_width_padded, sim_height_padded,
                           sim_depth_padded)
  glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

  // store to calculate the frame rate
  double last_update_time = glfwGetTime();
  int frame_count = 0;
  char title[TITLE_BUFFER_SIZE]; // buffer for the title so framerate is written
                                 // to the window title

  while (!glfwWindowShouldClose(window)) {
    // Asks the OS for events that happened since the last frame, inc. mouse
    // moves, the window being resized. It processes all queued events and fires
    // the relevant callbacks like framebuffer_size_callback
    glfwPollEvents();

    // Fixed timestep game loop: track how much time has elapsed and trigger the
    // simulation step only when a threshold is crossed. Every other iteration
    // just re-renders with the latest camera/view state.
    const double current_time = glfwGetTime();
    if (current_time - last_step_time >= step_time) {
      last_step_time = current_time;
      sim_step(sim, NULL);
      glNamedBufferSubData(render_info_buffer, 0,
                           (GLsizeiptr)sim_size *
                               (GLsizeiptr)sizeof(RenderInfo),
                           sim_render_info(sim));

      glUseProgram(occlusion_culling);
      DISPATCH_CULLING_COMPUTE(sim_width_padded, sim_height_padded,
                               sim_depth_padded)
      // Ensures writes to the occlusion SSBO are complete and visible to the
      // next compute shader that needs to read them
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Reset instance count
    glNamedBufferSubData(draw_indirect_buffer, sizeof(GLuint),
                         (GLsizeiptr)sizeof(GLuint), &zero);

    // Rebuild the view matrix using current camera position
    camera_position(&camera, eye);
    // Camera looks at the origin, and y-axis is up
    glm_lookat(eye, origin_coords, up_direction, view);
    camera_update_proj(fb_width, fb_height, bounding_radius, camera.radius,
                       proj);

    // Frustum culling
    glm_mat4_mul(proj, view, view_proj);
    glUseProgram(frustum_culling);
    frustum_extract(view_proj, eye);
    DISPATCH_CULLING_COMPUTE(sim_width_padded, sim_height_padded,
                             sim_depth_padded)

    if (opacity) {
      // For drawing
      glBindFramebuffer(GL_FRAMEBUFFER, blending_framebuffer);
      glClearBufferfv(GL_COLOR, ACCUM_BINDING_TARGET, ACCUM_CLEAR);
      glClearBufferfv(GL_COLOR, REVEAL_BINDING_TARGET, REVEAL_CLEAR);
      glBlendFunci(ACCUM_BINDING_TARGET, GL_ONE, GL_ONE);
      glBlendFunci(REVEAL_BINDING_TARGET, GL_ZERO, GL_ONE_MINUS_SRC_COLOR);
    } else {
      // The GPU maintains two buffers of the same pixel dimensions as your
      // window:
      // - Colour buffer: the RGB value of each pixel
      // - Depth buffer: the depth (z value after perspective divide, in 0–1
      //   range) of the closest fragment drawn to each pixel so far

      // At the start of each frame, the depth buffer still holds the values
      // from the previous frame.
      // - GL_COLOR_BUFFER_BIT — fill the colour buffer with the clear colour
      // - GL_DEPTH_BUFFER_BIT — fill the depth buffer with 1.0 everywhere

      // Clear the framebuffer drawn into
      glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    }

    // glDrawElementsIndirect works exactly like glDrawElementsInstanced,
    // except that the instance count is read from a buffer by the GPU instead
    // of being provided by the CPU.
    // The GPU processes 36 vertices per draw call, all belonging to one cube.
    // With instancing, the GPU processes 36 vertices for each instance on each
    // draw call. It takes per-instance data alongside the per-vertex data.
    glUseProgram(render_pipeline);
    glUniformMatrix4fv(view_proj_loc, 1, GL_FALSE, (float *)view_proj);
    glUniform3fv(camera_pos_loc, 1, eye);
    // Ensure that the compute shaders have completed its writes before reads
    // by the rendering pipeline
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT |
                    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_BYTE, 0);

    if (opacity) {
      // For final rendering
      glBindFramebuffer(GL_FRAMEBUFFER, WINDOW_FRAMEBUFFER_BINDING);
      // Clear the framebuffer rendered into
      glClear(GL_COLOR_BUFFER_BIT);
      glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

      glUseProgram(post_processing);
      glBindTextureUnit(ACCUM_BINDING_TARGET, accum_texture);
      glBindTextureUnit(REVEAL_BINDING_TARGET, reveal_texture);
      glDrawArrays(GL_TRIANGLES, 0, TRIANGLE_NUM_VERTICES);
    }

    // The GPU has 2 framebuffers:
    // - the back buffer that is drawn into
    // - the front buffer that is displayed
    // Swaps back and front buffer atomically
    glfwSwapBuffers(window);

    frame_count++;
    double elapsed_time = current_time - last_update_time;

    // update the framerate
    if (elapsed_time >= FPS_UPDATE_INTERVAL) {
      double fps = frame_count / elapsed_time;

      snprintf(title, TITLE_BUFFER_SIZE, "%s - FPS: %.0f", args.cell_path, fps);
      glfwSetWindowTitle(window, title);

      frame_count = 0;
      last_update_time = current_time;
    }
  }

  fprintf(stdout, "What you just saw is the simulation we live in...\n");
  sim_destroy(sim);
  dlclose(handle);
  glDeleteProgram(render_pipeline);
  glDeleteProgram(occlusion_culling);
  glDeleteProgram(frustum_culling);
  glDeleteProgram(post_processing);
  glDeleteVertexArrays(1, &attribute_buffer);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &element_buffer);
  glDeleteBuffers(1, &render_info_buffer);
  glDeleteBuffers(1, &occlusion_buffer);
  glDeleteBuffers(1, &instance_buffer);
  glDeleteBuffers(1, &draw_indirect_buffer);
  if (opacity) {
    glDeleteFramebuffers(1, &blending_framebuffer);
    glDeleteTextures(1, &accum_texture);
    glDeleteTextures(1, &reveal_texture);
  }
  DESTROY_AND_EXIT(window, true)
}

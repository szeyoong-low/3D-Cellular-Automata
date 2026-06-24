#include "args.h"
#include "cell_loader.h"
#include "simulate.h"
#include <dlfcn.h>
#include <glad/glad.h>
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
#include "frustum.h"
#include "graphics_utility.h"
#include "shader.h"

#define DESTROY_AND_EXIT(window, success)                                      \
  glfwDestroyWindow(window);                                                   \
  glfwTerminate();                                                             \
  return (success) ? EXIT_SUCCESS : EXIT_FAILURE;

#define TITLE_BUFFER_SIZE 64
#define FPS_UPDATE_INTERVAL 0.4

int main(int argc, char **argv) {
  args args = parse_args(argc, argv);

  uint initial_seed = args.custom_seed ? args.seed : (uint)time(NULL);
  printf("Initial seed used: %d\n", initial_seed);
  srand(initial_seed);

  void *handle;
  CellConfig cell_config = load_cell_config(args.cell_path, &handle);
  const uint sim_width = args.width;
  const uint sim_height = args.height;
  const uint sim_depth = args.depth;
  const size_t sim_size = sim_width * sim_height * sim_depth;
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
  Camera cam;
  // For bitonic sorting
  const float neg_inf = -HUGE_VALF;
  const ulong sim_size_padded = next_power_two(sim_size);
  const int sort_num_passes = (int)log2((double)sim_size_padded);
  // Radius of the smallest sphere that encloses the grid, from its centre.
  // Used to size the initial camera distance, FOV, and far clipping plane.
  const float bounding_radius =
      camera_grid_bounding_radius(sim_width, sim_height, sim_depth);

  glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

  // Start 2× the bounding radius away so the full grid fits in the FOV
  camera_init(&cam, window, 2.0F * bounding_radius);

  // Shaders are loaded from disk relative to the working directory.
  // Run the binary from the project root: ./bin/cellular_automata
  const GLuint render_pipeline =
      shader_build_program((ShaderDef[]){{GL_VERTEX_SHADER, VERTEX_SHADER},
                                         {GL_FRAGMENT_SHADER, FRAGMENT_SHADER}},
                           2);

  const GLuint hidden_cell_culling = shader_build_program(
      (ShaderDef[]){{GL_COMPUTE_SHADER, HIDDEN_CELL_COMPUTE_SHADER}}, 1);

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

  shader_upload_dim_uniforms(hidden_cell_culling, sim_width, sim_height,
                             sim_depth);
  shader_upload_dim_uniforms(frustum_culling, sim_width, sim_height, sim_depth);

  // VRAM buffer initialisation
  GLuint attribute_buffer, vertex_buffer, element_buffer, render_info_buffer,
      hidden_cell_buffer, instance_buffer, draw_indirect_buffer,
      sort_key_buffer;

  attribute_buffer_init(&attribute_buffer);
  vertex_buffer_init(&vertex_buffer);
  element_buffer_init(&element_buffer);
  render_info_buffer_init(&render_info_buffer, sim_size, sim_render_info(sim));
  hidden_cell_buffer_init(&hidden_cell_buffer, sim_size);
  instance_buffer_init(&instance_buffer,
                       (opacity) ? sim_size_padded : sim_size);
  draw_indirect_buffer_init(&draw_indirect_buffer);

  GLuint sort_global, sort_local;
  GLint sort_global_block_loc, sort_global_step_loc, sort_local_block_loc,
      sort_local_step_loc;

  if (opacity) {
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

    sort_global = shader_build_program(
        (ShaderDef[]){{GL_COMPUTE_SHADER, SORT_GLOBAL_COMPUTE_SHADER}}, 1);
    sort_global_block_loc =
        glGetUniformLocation(sort_global, SORT_BLOCK_UNIFORM);
    sort_global_step_loc = glGetUniformLocation(sort_global, SORT_STEP_UNIFORM);

    sort_local = shader_build_program(
        (ShaderDef[]){{GL_COMPUTE_SHADER, SORT_LOCAL_COMPUTE_SHADER}}, 1);
    sort_local_block_loc = glGetUniformLocation(sort_local, SORT_BLOCK_UNIFORM);
    sort_local_step_loc = glGetUniformLocation(sort_local, SORT_STEP_UNIFORM);

    sort_key_buffer_init(&sort_key_buffer, sim_size_padded);
  } else {
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);
  }

  double last_step_time = glfwGetTime(); // seconds as a double since glfwInit()
  glUseProgram(hidden_cell_culling);
  DISPATCH_CULLING_COMPUTE(sim_width, sim_height, sim_depth)
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

      glUseProgram(hidden_cell_culling);
      DISPATCH_CULLING_COMPUTE(sim_width, sim_height, sim_depth)
      // Ensures writes to the hidden_cells SSBO are complete and visible to the
      // next compute shader that needs to read them
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
    }

    // Reset instance count
    glNamedBufferSubData(draw_indirect_buffer, sizeof(GLuint),
                         (GLsizeiptr)sizeof(GLuint), &zero);

    // Rebuild the view matrix using current camera position
    camera_position(&cam, eye);
    // Camera looks at the origin, and y-axis is up
    glm_lookat(eye, origin_coords, up_direction, view);
    camera_update_proj(window, bounding_radius, cam.radius, proj);

    if (opacity) {
      glClearNamedBufferData(sort_key_buffer, GL_R32F, GL_RED, GL_FLOAT,
                             &neg_inf);
    }

    // Frustum culling
    glm_mat4_mul(proj, view, view_proj);
    glUseProgram(frustum_culling);
    frustum_extract(view_proj, eye);
    DISPATCH_CULLING_COMPUTE(sim_width, sim_height, sim_depth)

    // The GPU maintains two buffers of the same pixel dimensions as your
    // window:
    // - Colour buffer: the RGB value of each pixel
    // - Depth buffer: the depth (z value after perspective divide, in 0–1
    //   range) of the closest fragment drawn to each pixel so far

    // At the start of each frame, the depth buffer still holds the values from
    // the previous frame.
    // - GL_COLOR_BUFFER_BIT — fill the colour buffer with the clear colour
    // - GL_DEPTH_BUFFER_BIT — fill the depth buffer with 1.0 everywhere
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    if (opacity) {
      // Ensure that the frustum culler has completed its writes before reads
      // by the sorting algorithm
      glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

      // Sort instances back to front to address inconsistent alpha blending due
      // to z-fighting
      for (int i = 1; i <= sort_num_passes; i++) {
        const uint block_size = POWER_TWO(i);

        for (int j = i - 1; j >= 0; j--) {
          const uint step_size = POWER_TWO(j);
          // Swap partner is within shared memory
          const bool local = step_size <= SORTING_LOCAL_MAX_STEP;

          glUseProgram(local ? sort_local : sort_global);
          glUniform1ui(local ? sort_local_block_loc : sort_global_block_loc,
                       block_size);
          glUniform1ui(local ? sort_local_step_loc : sort_global_step_loc,
                       step_size);
          glDispatchCompute(NUM_WORKERS(sim_size_padded, SORTING_LOCAL_SIZE),
                            SORTING_NUM_WORKERS_Y, SORTING_NUM_WORKERS_Z);
          // This is a global memory barrier
          glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
          if (local) {
            break; // All smaller step sizes taken care of
          }
        }
      }
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
    // Ensure that the sorting algorithm has completed its writes before reads
    // by the rendering pipeline
    glMemoryBarrier(GL_COMMAND_BARRIER_BIT |
                    GL_VERTEX_ATTRIB_ARRAY_BARRIER_BIT);
    glDrawElementsIndirect(GL_TRIANGLES, GL_UNSIGNED_BYTE, 0);

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
  glDeleteProgram(hidden_cell_culling);
  glDeleteProgram(frustum_culling);
  glDeleteVertexArrays(1, &attribute_buffer);
  glDeleteBuffers(1, &vertex_buffer);
  glDeleteBuffers(1, &element_buffer);
  glDeleteBuffers(1, &render_info_buffer);
  glDeleteBuffers(1, &hidden_cell_buffer);
  glDeleteBuffers(1, &instance_buffer);
  glDeleteBuffers(1, &draw_indirect_buffer);
  if (opacity) {
    glDeleteProgram(sort_global);
    glDeleteProgram(sort_local);
    glDeleteBuffers(1, &sort_key_buffer);
  }
  DESTROY_AND_EXIT(window, true)
}

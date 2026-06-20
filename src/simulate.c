#include "simulate.h"
#include <assert.h>
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#define VERIFY(ptr) assert(ptr != NULL)

struct Sim {
  // simulation dimensions and buffers
  uint width, height, depth;
  size_t cell_size;

  // byte addressable memory buffers for cells
  // this approach gives the ability to simulate different
  // types of cells, which are only known at runtime
  uint8_t *curr_state; // read-only
  uint8_t *next_state; // write-only

  // array of RenderInfo structs
  // allows the renderer to get info for each cell and render it
  RenderInfo *renderer_state;

  // concurrency fields
  bool is_running;
  size_t num_threads;
  pthread_t *threads;

  // atomic counter for array slice assignment in process_slice
  atomic_uint curr_slice;

  // atomic bool for checking whether we are initialising or updating
  atomic_bool is_initialising;

  // barriers (gates) for thread syncing
  pthread_barrier_t start_barrier, end_barrier;

  // init cell protocol
  InitialiseCell init_cell;

  // update cell protocol
  UpdateCell update_cell;
};

// allows threads to process the slices concurrently
// the slice is determined by the curr_slice counter
// return: NULL (default)
static void *process_slice(void *arg);

// finds out the maximum number of available threads at runtime
// returns: max number of available threads,
//          accounting for 1 thread taking up main
static uint max_threads(void);

Sim sim_create(uint width, uint height, uint depth, size_t num_threads,
               CellConfig cell_config) {
  const uint num_cells = width * height * depth;
  const size_t state_size = num_cells * cell_config.cell_size;
  num_threads = num_threads == 0 ? max_threads() : num_threads;

  Sim simulation = malloc(sizeof(struct Sim));
  VERIFY(simulation);

  simulation->is_running = true;
  simulation->width = width;
  simulation->height = height;
  simulation->depth = depth;
  cell_config.dims_init(width, height, depth);
  simulation->cell_size = cell_config.cell_size;
  simulation->num_threads = num_threads;

  VERIFY(cell_config.cell_init);
  simulation->init_cell = cell_config.cell_init;
  VERIFY(cell_config.cell_update);
  simulation->update_cell = cell_config.cell_update;

  simulation->curr_state = malloc(state_size);
  VERIFY(simulation->curr_state);
  simulation->next_state = malloc(state_size);
  VERIFY(simulation->next_state);
  simulation->renderer_state = malloc(num_cells * sizeof(RenderInfo));
  VERIFY(simulation->renderer_state);

  // barrier initialisation. +1 threads comes from the
  // fact that the main thread has to be taken into account
  pthread_barrier_init(&simulation->start_barrier, NULL, (uint)num_threads + 1);
  pthread_barrier_init(&simulation->end_barrier, NULL, (uint)num_threads + 1);

  simulation->threads = malloc(num_threads * sizeof(pthread_t));
  VERIFY(simulation->threads);

  // creation of threads which infinitely run process_slice
  for (uint i = 0; i < num_threads; i++) {
    pthread_create(&simulation->threads[i], NULL, process_slice, simulation);
  }

  // trigger the init loop
  simulation->is_initialising = true;
  simulation->curr_slice = 0;
  pthread_barrier_wait(&simulation->start_barrier);
  pthread_barrier_wait(&simulation->end_barrier);
  simulation->is_initialising = false;

  return simulation;
}

void sim_destroy(Sim sim) {
  VERIFY(sim);

  // stop the simulation and wake all threads
  sim->is_running = false;
  // this barrier ensures all threads are waiting
  // at the start of process slice function
  pthread_barrier_wait(&sim->start_barrier);

  // rejoin the threads together
  // the threads must be woken up before they can be rejoined
  for (uint i = 0; i < sim->num_threads; i++) {
    pthread_join(sim->threads[i], NULL);
  }

  // destroy the barriers
  pthread_barrier_destroy(&sim->start_barrier);
  pthread_barrier_destroy(&sim->end_barrier);

  // freeing malloc'd space
  free(sim->curr_state);
  free(sim->next_state);
  free(sim->renderer_state);
  free(sim->threads);
  free(sim);
}

const RenderInfo *sim_render_info(Sim sim) {
  VERIFY(sim);
  return sim->renderer_state;
}

void sim_step(Sim sim, UpdateCell update_cell) {
  VERIFY(sim);

  if (update_cell != NULL) {
    // update the "cell update protocol"
    sim->update_cell = update_cell;
  }

  // prepare the simulation to the next step
  sim->curr_slice = 0;

  // open the barrier and wait for threads to update the simulation
  // the start barrier is also present in process slice. When the main
  // thread gets to the start barrier, it's opened, allowing worker
  // threads to update the state, until they hit the end barrier.
  // the end barrier is released when the slowest thread arrives to it
  pthread_barrier_wait(&sim->start_barrier);
  pthread_barrier_wait(&sim->end_barrier);

  // swap the buffers to update the state and get ready for the next cycle
  void *temp = sim->curr_state;
  sim->curr_state = sim->next_state;
  sim->next_state = temp;
}

static void *process_slice(void *arg) {
  Sim sim = arg;

  while (true) {
    // this barrier keeps the worker threads waiting
    // it is only released when the main thread hits
    // it in the step function
    // as a result, the threads are synchronised together
    pthread_barrier_wait(&sim->start_barrier);

    // check whether simulation finished whilst the thread was waiting
    if (!sim->is_running) {
      break;
    }

    while (true) {
      // get next slice depth
      uint slice = atomic_fetch_add(&sim->curr_slice, 1);
      if (slice >= sim->depth) {
        break;
      }

      const uint slice_offset = slice * sim->width * sim->height;

      // we can check if initialising here as this is consistent for a slice
      // no threads will modify this value so we do not need an atomic load
      bool initialising = sim->is_initialising;

      for (uint row = 0; row < sim->height; row++) {
        const uint row_offset = slice_offset + (row * sim->width);

        for (uint col = 0; col < sim->width; col++) {
          const uint curr_id = row_offset + col;

          if (initialising) {
            sim->init_cell(&sim->curr_state[curr_id * sim->cell_size],
                           &sim->renderer_state[curr_id], col, row, slice);
          } else {
            sim->update_cell(&sim->next_state[curr_id * sim->cell_size],
                             &sim->renderer_state[curr_id], sim->curr_state,
                             col, row, slice);
          }
        }
      }
    }
    // similarly to the start barrier, the end barrier is released
    // when the slowest thread hits it. As a result, the computation
    // time is limited by the slowest thread.
    pthread_barrier_wait(&sim->end_barrier);
  }
  return NULL;
}

static uint max_threads(void) {
  return (uint)sysconf(_SC_NPROCESSORS_ONLN) - 1;
}

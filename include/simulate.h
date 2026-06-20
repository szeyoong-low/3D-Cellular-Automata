#ifndef SIMULATE_H
#define SIMULATE_H

#include "extension_types.h"

// NOTE: all functions abort programme on memory allocation failures

typedef struct Sim *Sim;

typedef struct {
  size_t cell_size;
  UpdateCell cell_update;
  InitialiseCell cell_init;
  InitialiseDims dims_init;
} CellConfig;

// note: if num_threads is 0, the number of threads is equal to the number of
// threads the program can use from the processor returns: a simulation with the
// provided dimensions, worker threads count,
//          and mandatory cell update protocol. The cells at positions specified
//          in set_cells are set
extern Sim sim_create(uint width, uint height, uint depth, size_t num_threads,
                      CellConfig cell_config);

// side effect: updates the simulation's cells based on the simulation's rules
// precondition: must be a valid simulation created via sim_create
// note: sim_step is not thread-safe. update_cell is optional (so can be NULL).
//       caller must ensure update_cell is compatible with the type of cells
//       already in the simulation
extern void sim_step(Sim sim, UpdateCell update_cell);

// return: read-only 3rd order tensor of information to render each of the
//         simulation's cells
// precondition: must be a valid simulation created via sim_create
extern const RenderInfo *sim_render_info(Sim sim);

// precondition: must be a valid simulation created via sim_create
// frees memory allocated for the simulation
// MUST be called before programme exit
extern void sim_destroy(Sim sim);

#endif

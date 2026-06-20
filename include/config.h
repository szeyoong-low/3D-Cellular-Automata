#ifndef CONFIG_H
#define CONFIG_H
// These functions must be implemented for custom simulation
#include "extension_types.h"

// a macro that can be used to calculate a grid index

// x is column, y is row, z is slice, w is width, h is height
#define INDEX(x, y, z, w, h)                                                   \
  (((uint)x) + ((uint)w) * (((uint)y) + ((uint)h) * ((uint)z)))

// The size in bytes of the custom cell
extern const size_t cell_size;

// passes dimensions of the simulation to the custom config
// note: it is user responsibility to keep track of hte
extern void dims_init(uint width, uint height, uint depth);

// initialises the cell in the simulation and renderer states
extern void cell_init(void *cell_to_update, RenderInfo *renderer_state,
                      uint col, uint row, uint slice);

// updates the cell based on the current simulation state
// any changes to renderer states must be explicitly defined in the function
extern void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                        const void *curr_sim_state, uint col, uint row,
                        uint slice);
#endif

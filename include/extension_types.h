#ifndef EXTENSION_TYPES_H
#define EXTENSION_TYPES_H

#include <inttypes.h>
#include <sys/types.h>

#define MAX_RGBA_VAL 255

typedef struct {
  uint8_t red;
  uint8_t green;
  uint8_t blue;
  uint8_t alpha;
} RenderInfo;

// sends the dimensions of the simulation to the cell config
// allowing it to know the size of the grid and do bounds checks
// or anything requiring the size of the grid
typedef void (*InitialiseDims)(uint width, uint height, uint depth);

// side effects: initialises internal(simulation) and external(renderer)
//               representations of a cell at a position (column, row, slice)
typedef void (*InitialiseCell)(void *simulation_state,
                               RenderInfo *renderer_state, uint col, uint row,
                               uint slice);

// definition of simulation rules
// (column, row, slice) are the coordinates of a point to be updated
// side effects: updates the simulation cell pointer passed in to get the
//               next state of the cell
typedef void (*UpdateCell)(void *cell_to_update, RenderInfo *renderer_state,
                           const void *curr_sim_state, uint col, uint row,
                           uint slice);

#endif

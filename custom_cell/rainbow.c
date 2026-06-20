#include "config.h"
#include "extension_types.h"
#include <stdlib.h>

#define NUM_STATES 12
#define ADVANCE_THRESHOLD 3

typedef uint8_t RainbowCell;

const size_t cell_size = sizeof(RainbowCell);

static uint grid_width, grid_height, grid_depth;

static inline RainbowCell get_cell_state(const RainbowCell *grid, int x, int y,
                                         int z) {
  // wrap reads around to the other side if out of bounds
  x = ((x % (int)grid_width) + (int)grid_width) % (int)grid_width;
  y = ((y % (int)grid_height) + (int)grid_height) % (int)grid_height;
  z = ((z % (int)grid_depth) + (int)grid_depth) % (int)grid_depth;
  return grid[INDEX(x, y, z, grid_width, grid_height)];
}

// the list of colours to cycle through depending on the state
static const uint8_t PALETTE[NUM_STATES][3] = {
    {255, 0, 0}, {255, 128, 0}, {255, 255, 0}, {128, 255, 0},
    {0, 255, 0}, {0, 255, 128}, {0, 255, 255}, {0, 128, 255},
    {0, 0, 255}, {128, 0, 255}, {255, 0, 255}, {255, 0, 128},
};

static inline void assign_colour(RenderInfo *renderer_state,
                                 RainbowCell state) {
  renderer_state->red = PALETTE[state][0];
  renderer_state->green = PALETTE[state][1];
  renderer_state->blue = PALETTE[state][2];
  renderer_state->alpha = MAX_RGBA_VAL;
}

void dims_init(uint width, uint height, uint depth) {
  grid_width = width;
  grid_height = height;
  grid_depth = depth;
}

void cell_init(void *cell_to_update, RenderInfo *renderer_state, uint col,
               uint row, uint slice) {
  (void)col;
  (void)row;
  (void)slice;

  RainbowCell *cell = cell_to_update;
  // give each cell a random state
  *cell = (RainbowCell)(rand() % NUM_STATES);
  assign_colour(renderer_state, *cell);
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {
  RainbowCell *next = cell_to_update;
  const RainbowCell *grid = curr_sim_state;
  const RainbowCell curr_cell_state =
      get_cell_state(grid, (int)col, (int)row, (int)slice);

  // this is the next state (i.e. 1 if a cell is in state 0)
  RainbowCell successor_state =
      (RainbowCell)((curr_cell_state + 1) % NUM_STATES);

  size_t count = 0;
  for (int dz = -1; dz <= 1; dz++) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        if (dx == 0 && dy == 0 && dz == 0) {
          continue;
        }
        // count the number of neighbour successor cells
        if (get_cell_state(grid, (int)col + dx, (int)row + dy,
                           (int)slice + dz) == successor_state) {
          count++;
        }
      }
    }
  }

  // if a cell is surrounded by at least the threshold of successor cells,
  // it will itself move on to the next state
  *next = (count >= ADVANCE_THRESHOLD) ? successor_state : curr_cell_state;
  assign_colour(renderer_state, *next);
}

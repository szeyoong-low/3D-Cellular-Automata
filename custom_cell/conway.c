#include "config.h"
#include "extension_types.h"
#include <stdbool.h>
#include <stdlib.h>

#define SURVIVE_RULE_1 4
#define SURVIVE_RULE_2 5
#define BIRTH_RULE_1 5
#define BIRTH_RULE_2 5
#define START_PERCENT 10

// an implementation of conway's game of life adapted for 3D
// uses the 5766 rules from this paper:
// https://content.wolfram.com/sites/13/2018/02/01-3-1.pdf 5766
// means: Birth 6 or 6: a dead cell comes to life if it has 6 neighbours
// Survival 5 or 7: an alive cell only survives if it has 5 or 7 neighbours

// true represents an alive cell
typedef bool ConwayCell;

const size_t cell_size = sizeof(ConwayCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;

static inline ConwayCell get_cell_state(const ConwayCell *curr_state, int x,
                                        int y, int z);

static inline size_t count_alive_neighbours(const ConwayCell *curr_state,
                                            uint x, uint y, uint z);

static inline void assign_colour(RenderInfo *renderer_state,
                                 ConwayCell cell_state);

ConwayCell get_cell_state(const ConwayCell *curr_state, int x, int y, int z) {
  if (x < 0 || x >= (int)grid_width || y < 0 || y >= (int)grid_height ||
      z < 0 || z >= (int)grid_depth) {
    return false;
  }

  uint index = INDEX(x, y, z, grid_width, grid_height);
  return curr_state[index];
}

size_t count_alive_neighbours(const ConwayCell *curr_state, uint x, uint y,
                              uint z) {
  size_t count = 0;
  int base_x = (int)x;
  int base_y = (int)y;
  int base_z = (int)z;

  for (int dz = -1; dz <= 1; dz++) {
    for (int dy = -1; dy <= 1; dy++) {
      for (int dx = -1; dx <= 1; dx++) {
        // do not count the target cell
        if (dx == 0 && dy == 0 && dz == 0)
          continue;

        if (get_cell_state(curr_state, base_x + dx, base_y + dy, base_z + dz)) {
          count++;
        }
      }
    }
  }

  return count;
}

void assign_colour(RenderInfo *renderer_state, ConwayCell cell_state) {
  if (cell_state) {
    renderer_state->red = 0;
    renderer_state->green = MAX_RGBA_VAL;
    renderer_state->blue = 0;
    renderer_state->alpha = MAX_RGBA_VAL;
  } else {
    renderer_state->red = 0;
    renderer_state->green = 0;
    renderer_state->blue = 0;
    renderer_state->alpha = 0;
  }
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

  ConwayCell *cell = cell_to_update;

  *cell = (rand() % 100) < START_PERCENT;
  assign_colour(renderer_state, *cell);
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {
  ConwayCell *conway_cell_to_update = cell_to_update;
  const ConwayCell *curr_grid = curr_sim_state;

  ConwayCell curr_cell_state =
      get_cell_state(curr_grid, (int)col, (int)row, (int)slice);
  size_t neighbours = count_alive_neighbours(curr_grid, col, row, slice);

  if (curr_cell_state) {
    *conway_cell_to_update =
        (neighbours == SURVIVE_RULE_1 || neighbours == SURVIVE_RULE_2);
  } else {
    *conway_cell_to_update =
        (neighbours == BIRTH_RULE_1 || neighbours == BIRTH_RULE_2);
  }

  assign_colour(renderer_state, *conway_cell_to_update);
}

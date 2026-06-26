#include "config.h"
#include "extension_types.h"
#include <stdbool.h>
#include <stdlib.h>

// follows the rules from here:
// https://www.roguebasin.com/index.php/Cellular_Automata_Method_for_Generating_Random_Cave-Like_Levels

#define STAY_WALL_MIN 4
#define BECOME_WALL_MIN 5
#define START_PERCENT 45

typedef bool CaveCell;

const size_t cell_size = sizeof(CaveCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;

static int min_dx, max_dx;
static int min_dy, max_dy;
static int min_dz, max_dz;

static inline CaveCell get_cell_state(const CaveCell *curr_state, int x, int y,
                                      int z);

static inline size_t count_alive_neighbours(const CaveCell *curr_state, uint x,
                                            uint y, uint z);

static inline void assign_colour(RenderInfo *renderer_state,
                                 CaveCell cell_state);

CaveCell get_cell_state(const CaveCell *curr_state, int x, int y, int z) {
  if (x < 0 || x >= (int)grid_width || y < 0 || y >= (int)grid_height ||
      z < 0 || z >= (int)grid_depth) {
    return true;
  }

  uint index = INDEX(x, y, z, grid_width, grid_height);
  return curr_state[index];
}

size_t count_alive_neighbours(const CaveCell *curr_state, uint x, uint y,
                              uint z) {
  size_t count = 0;
  int base_x = (int)x;
  int base_y = (int)y;
  int base_z = (int)z;

  for (int dz = min_dz; dz <= max_dz; dz++) {
    for (int dy = min_dy; dy <= max_dy; dy++) {
      for (int dx = min_dx; dx <= max_dx; dx++) {
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

void assign_colour(RenderInfo *renderer_state, CaveCell cell_state) {
  if (cell_state) {
    renderer_state->red = 0x6A;
    renderer_state->green = 0x67;
    renderer_state->blue = 0x67;
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

  min_dx = (width == 1) ? 0 : -1;
  max_dx = (width == 1) ? 0 : 1;

  min_dy = (height == 1) ? 0 : -1;
  max_dy = (height == 1) ? 0 : 1;

  min_dz = (depth == 1) ? 0 : -1;
  max_dz = (depth == 1) ? 0 : 1;
}

void cell_init(void *cell_to_update, RenderInfo *renderer_state, uint col,
               uint row, uint slice) {
  (void)col;
  (void)row;
  (void)slice;

  CaveCell *cell = cell_to_update;

  *cell = (rand() % 100) < START_PERCENT;
  assign_colour(renderer_state, *cell);
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {
  CaveCell *cave_cell_to_update = cell_to_update;
  const CaveCell *curr_grid = curr_sim_state;

  CaveCell curr_cell_state =
      get_cell_state(curr_grid, (int)col, (int)row, (int)slice);
  size_t neighbours = count_alive_neighbours(curr_grid, col, row, slice);

  if (curr_cell_state) {
    *cave_cell_to_update = (neighbours >= STAY_WALL_MIN);
  } else {
    *cave_cell_to_update = (neighbours >= BECOME_WALL_MIN);
  }

  assign_colour(renderer_state, *cave_cell_to_update);
}

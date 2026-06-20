#include "config.h"
#include "extension_types.h"

typedef uint8_t EmptyCell;

const size_t cell_size = sizeof(EmptyCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;

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

  EmptyCell *cell = cell_to_update;
  *cell = 0;

  renderer_state->alpha = 0;
  renderer_state->red = 0;
  renderer_state->green = 0;
  renderer_state->blue = 0;
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {
  (void)curr_sim_state;
  (void)col;
  (void)row;
  (void)slice;

  cell_init(cell_to_update, renderer_state, col, row, slice);
}

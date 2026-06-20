#include "config.h"
#include <stdio.h>

#define ALPHA 100;

typedef uint8_t EmptyCell;

const size_t cell_size = sizeof(EmptyCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;

void dims_init(uint width, uint height, uint depth) {
  grid_height = height;
  grid_width = width;
  grid_depth = depth;
}

void cell_init(void *cell_to_update, RenderInfo *renderer_state, uint col,
               uint row, uint slice) {
  (void)row;
  (void)slice;

  EmptyCell *cell = cell_to_update;
  *cell = 0;

  // assign a unique colour to each octant
  if (col < grid_width / 2 && row < grid_height / 2 && slice < grid_depth / 2) {
    // Green
    renderer_state->alpha = ALPHA;
    renderer_state->red = 0;
    renderer_state->green = MAX_RGBA_VAL;
    renderer_state->blue = 0;

  } else if (col >= grid_width / 2 && row < grid_height / 2 &&
             slice < grid_depth / 2) {
    // Red
    renderer_state->alpha = ALPHA;
    renderer_state->red = MAX_RGBA_VAL;
    renderer_state->green = 0;
    renderer_state->blue = 0;

  } else if (col < grid_width / 2 && row >= grid_height / 2 &&
             slice < grid_depth / 2) {
    // Blue
    renderer_state->alpha = ALPHA;
    renderer_state->red = 0;
    renderer_state->green = 0;
    renderer_state->blue = MAX_RGBA_VAL;

  } else if (col >= grid_width / 2 && row >= grid_height / 2 &&
             slice < grid_depth / 2) {
    // Yellow
    renderer_state->alpha = ALPHA;
    renderer_state->red = MAX_RGBA_VAL;
    renderer_state->green = MAX_RGBA_VAL;
    renderer_state->blue = 0;

  } else if (col < grid_width / 2 && row < grid_height / 2 &&
             slice >= grid_depth / 2) {
    // Cyan
    renderer_state->alpha = ALPHA;
    renderer_state->red = 0;
    renderer_state->green = MAX_RGBA_VAL;
    renderer_state->blue = MAX_RGBA_VAL;

  } else if (col >= grid_width / 2 && row < grid_height / 2 &&
             slice >= grid_depth / 2) {
    // Magenta
    renderer_state->alpha = ALPHA;
    renderer_state->red = MAX_RGBA_VAL;
    renderer_state->green = 0;
    renderer_state->blue = MAX_RGBA_VAL;

  } else if (col < grid_width / 2 && row >= grid_height / 2 &&
             slice >= grid_depth / 2) {
    // White
    renderer_state->alpha = ALPHA;
    renderer_state->red = MAX_RGBA_VAL;
    renderer_state->green = MAX_RGBA_VAL;
    renderer_state->blue = MAX_RGBA_VAL;

  } else {
    // Black
    renderer_state->alpha = ALPHA;
    renderer_state->red = 0;
    renderer_state->green = 0;
    renderer_state->blue = 0;
  }
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {
  (void)curr_sim_state;
  (void)cell_to_update;
  (void)renderer_state;
  (void)col;
  (void)row;
  (void)slice;
}

#include "config.h"
#include "extension_types.h"
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>

// 0 for only the cell in the centre being checked
#define DENSITY_CHECK_BEYOND_CELL 1
#define DENSITY_CHECK_VOLUME pow(2 * DENSITY_CHECK_BEYOND_CELL + 1, 3)

#define GROWTH_CHECK_BEYOND_CELL 1

#define GROWTH_THRESHOLD 0.45f
#define LIQUID_CONCENTRATION 1.0f
#define ALIGNMENT_NUDGE 0.1f

// 1 for everything opaque, above that is transparent
#define OPACITY_FRACTION_PRESERVED 0.98f

#define APPROX_START_NUMBER_OF_NUCLEATION_SITES 2

#define RAND_UNIT ((rand_float() * 2.0f) - 1.0f)

typedef enum { AIR, LIQUID, CRYSTAL } CrystalState;

typedef struct {
  CrystalState state;
  float ox, oy, oz;
  float opacity;
} CrystalCell;

#define AIR_CELL ((CrystalCell){AIR, 0.0f, 0.0f, 0.0f, 0.0f})

const size_t cell_size = sizeof(CrystalCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;

static float start_crystal_chance;

static inline CrystalCell get_cell(const CrystalCell *grid, int x, int y,
                                   int z);

static inline float rand_float(void) { return (float)rand() / (float)RAND_MAX; }

static void normalise_vector(float *x, float *y, float *z) {
  float len = sqrtf((*x) * (*x) + (*y) * (*y) + (*z) * (*z));

  // in case we need to divide by 0
  // this part of code should not run often at all
  if (len < 0.001f) {
    *x = 1.0f;
    *y = 0.0f;
    *z = 0.0f;
    return;
  }

  *x /= len;
  *y /= len;
  *z /= len;
}

static void random_orientation(float *x, float *y, float *z) {
  *x = RAND_UNIT;
  *y = RAND_UNIT;
  *z = RAND_UNIT;

  normalise_vector(x, y, z);
}

static void nudge_orientation(float *x, float *y, float *z) {
  *x += RAND_UNIT * ALIGNMENT_NUDGE;
  *y += RAND_UNIT * ALIGNMENT_NUDGE;
  *z += RAND_UNIT * ALIGNMENT_NUDGE;

  normalise_vector(x, y, z);
}

CrystalCell get_cell(const CrystalCell *grid, int x, int y, int z) {
  if (x < 0 || x >= (int)grid_width || y < 0 || y >= (int)grid_height ||
      z < 0 || z >= (int)grid_depth) {
    return AIR_CELL;
  }

  return grid[INDEX((uint)x, (uint)y, (uint)z, grid_width, grid_height)];
}

static bool try_growth(const CrystalCell *grid, CrystalCell *out, uint x,
                       uint y, uint z) {
  float total_score = 0.0f;
  float best_score = 0.0f;
  CrystalCell best_parent = AIR_CELL;

  for (int dz = -GROWTH_CHECK_BEYOND_CELL; dz <= GROWTH_CHECK_BEYOND_CELL;
       dz++) {
    for (int dy = -GROWTH_CHECK_BEYOND_CELL; dy <= GROWTH_CHECK_BEYOND_CELL;
         dy++) {
      for (int dx = -GROWTH_CHECK_BEYOND_CELL; dx <= GROWTH_CHECK_BEYOND_CELL;
           dx++) {

        int new_x = (int)x + dx;
        int new_y = (int)y + dy;
        int new_z = (int)z + dz;

        CrystalCell n = get_cell(grid, new_x, new_y, new_z);

        if (n.state != CRYSTAL)
          continue;

        float len = sqrtf((float)(dx * dx + dy * dy + dz * dz));

        float dir_x = -(float)dx / len;
        float dir_y = -(float)dy / len;
        float dir_z = -(float)dz / len;

        float alignment = n.ox * dir_x + n.oy * dir_y + n.oz * dir_z;

        if (alignment < 0.0f)
          alignment = 0.0f;

        float score = alignment;

        total_score += score;

        if (score > best_score) {
          best_score = score;
          best_parent = n;
        }
      }
    }
  }

  if (total_score <= 0.0f)
    return false;

  if (total_score >= GROWTH_THRESHOLD) {
    out->state = CRYSTAL;
    out->ox = best_parent.ox;
    out->oy = best_parent.oy;
    out->oz = best_parent.oz;
    nudge_orientation(&out->ox, &out->oy, &out->oz);
    out->opacity = best_parent.opacity * OPACITY_FRACTION_PRESERVED;
    return true;
  }

  return false;
}

static float local_density(const CrystalCell *grid, uint x, uint y, uint z) {
  uint density = 0;

  for (int dz = -DENSITY_CHECK_BEYOND_CELL; dz <= DENSITY_CHECK_BEYOND_CELL;
       dz++) {
    for (int dy = -DENSITY_CHECK_BEYOND_CELL; dy <= DENSITY_CHECK_BEYOND_CELL;
         dy++) {
      for (int dx = -DENSITY_CHECK_BEYOND_CELL; dx <= DENSITY_CHECK_BEYOND_CELL;
           dx++) {

        CrystalCell n = get_cell(grid, (int)x + dx, (int)y + dy, (int)z + dz);

        if (n.state == CRYSTAL) {
          density++;
        }
      }
    }
  }

  return (float)density / (float)DENSITY_CHECK_VOLUME;
}

static void assign_colour(RenderInfo *renderer_state, CrystalCell cell) {
  renderer_state->red = (uint8_t)(fabsf(cell.ox) * MAX_RGBA_VAL);
  renderer_state->green = (uint8_t)(fabsf(cell.oy) * MAX_RGBA_VAL);
  renderer_state->blue = (uint8_t)(fabsf(cell.oz) * MAX_RGBA_VAL);
  renderer_state->alpha = (uint8_t)(MAX_RGBA_VAL * cell.opacity);
}

void dims_init(uint width, uint height, uint depth) {
  grid_width = width;
  grid_height = height;
  grid_depth = depth;

  start_crystal_chance = (float)APPROX_START_NUMBER_OF_NUCLEATION_SITES /
                         (float)(width * height * depth);
}

void cell_init(void *cell_to_update, RenderInfo *renderer_state, uint col,
               uint row, uint slice) {

  (void)col;
  (void)row;
  (void)slice;

  CrystalCell *cell = cell_to_update;

  cell->state = LIQUID;
  cell->ox = 0.0f;
  cell->oy = 0.0f;
  cell->oz = 0.0f;
  cell->opacity = 0.0f;

  if (rand_float() < start_crystal_chance) {
    cell->state = CRYSTAL;
    cell->opacity = 1.0f;
    random_orientation(&cell->ox, &cell->oy, &cell->oz);
  } else if (rand_float() > LIQUID_CONCENTRATION) {
    cell->state = AIR;
  }

  assign_colour(renderer_state, *cell);
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {

  CrystalCell *next = cell_to_update;
  const CrystalCell *grid = curr_sim_state;

  CrystalCell curr = get_cell(grid, (int)col, (int)row, (int)slice);

  *next = curr;

  if (curr.state == LIQUID && local_density(grid, col, row, slice) < 0.1) {
    try_growth(grid, next, col, row, slice);
  }

  assign_colour(renderer_state, *next);
}

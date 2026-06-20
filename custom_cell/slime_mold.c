#include "config.h"
#include "extension_types.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/types.h>

//  inspiration taken from
//  https://youtube.com/shorts/cuIjdiJoMDg?si=uAzjxO9nw27N6PJi

#define MAX_PHEROMONE 255.0F
#define MAX_NUTRIENTS 255.0F
#define MAX_SCENT 255.0F
// rate at which pheromones evaporate every step
#define PHEROMONE_DECAY_AMOUNT 3.5F
// dynamic decay rate is calculated with:
// dyn_decay = decay_amount + rand(0, decay_jitter)
#define PHEROMONE_DECAY_JITTER 12
// rate at which pheromones diffuse to neighbours
#define PHEROMONE_DIFFUSION_COEFF 0.6F
// determines how much pheromon is retained during diffusion
// i.e. 1 - RETENTION_RATE = % pheromone lost during diffusion
#define PHEROMONE_RETENTION_RATE 0.85F
// scent coefficients
#define SCENT_DECAY_AMOUNT 12.0F
#define SCENT_DECAY_JITTER 10
#define SCENT_DIFFUSION_COEFF 0.85F
#define SCENT_RETENTION_RATE 0.40F
// multiplier of how aggressively mold pioritises scent
#define SCENT_WEIGHT 1.5F
// mold ignores scent below the threshold
#define SCENT_NOISE_FLOOR 15.0F
// base pheromone threshold at which the cell becomes mold
// to simulate random nature of cell formation,
// a jitter is added to the threshold
#define MOLD_THRESHOLD 50.0F
// jitter sets the maximum the threshold can be increased by
// dynamic mold threshold is calculated with:
// dyn_threshold = mold_threshold + rand(0, mold_jitter)
#define MOLD_JITTER 110
// rate of nutrient consumption of the cell which is feeding
#define CONSUMPTION_RATE 1.0F
// starting nutrient levels for initial mold cells
#define MOLD_NUTRIENTS 50.0F
// food spawn rate = hits / total
#define FOOD_SPAWN_HITS 1
#define FOOD_SPAWN_TOTAL 10000
// mold spawn rate = hits / total
#define MOLD_SPAWN_HITS 2
#define MOLD_SPAWN_TOTAL 10000
// maximum initial mold and food cells allowed
#define MAX_MOLD_PER_SIM 1
#define MAX_FOOD_PER_SIM 5

#define MOLD_COLOUR(opacity)                                                   \
  (RenderInfo) { MAX_RGBA_VAL, 200, 0, opacity }
#define PHEROMONE_COLOUR                                                       \
  (RenderInfo) { MAX_RGBA_VAL, 192, 203, 20 }
#define FOOD_COLOUR                                                            \
  (RenderInfo) { 0, 180, MAX_RGBA_VAL, MAX_RGBA_VAL }
#define EMPTY_COLOUR (RenderInfo){0, 0, 0, 0};

typedef struct {
  float nutrient_level;
  float pheromone_level;
  float scent_level;
} MoldCell;

const size_t cell_size = sizeof(MoldCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;

// side-effect: modifies render info
// desc: sets the colour of the cell depending on it's nutrition and
//       pheromone levels
//  active mold -> bright orange/yellow
//  pheromone trail -> trnsluscent pink
//  food -> cyan
//  dead/empty space -> invisible
static void assign_colour(const MoldCell *cell, RenderInfo *renderer);

// desc: provides safe access to the simulation cell
// returns: NULL if the indecies are Out of Bounds
//          pointer to the valid cell in the grid
static inline const MoldCell *get_cell_safe(const MoldCell *sim_grid, int col,
                                            int row, int slice);

// updates dynamic thresholds for classification of the cell as mold
// and pheromone evaproration amount
static void update_thresholds(void);

static float dynamic_mold_threshold;
static float dynamic_pheromone_decay_amount;
static float dynamic_scent_decay_amount;

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
  // initialise the thresholds
  update_thresholds();

  static uint curr_mold_cells = 0;
  static uint curr_food_cells = 0;
  MoldCell *cell = cell_to_update;

  // initialise the cell fields
  cell->nutrient_level = 0;
  cell->pheromone_level = 0;
  cell->scent_level = 0;

  if ((rand() % MOLD_SPAWN_TOTAL < MOLD_SPAWN_HITS) &&
      curr_mold_cells < MAX_MOLD_PER_SIM) {
    // spawn mold
    cell->pheromone_level = MAX_PHEROMONE;
    cell->nutrient_level = MOLD_NUTRIENTS;
    curr_mold_cells++;
  } else if ((rand() % FOOD_SPAWN_TOTAL < FOOD_SPAWN_HITS) &&
             curr_food_cells < MAX_FOOD_PER_SIM) {
    // spawn food cell
    cell->nutrient_level = MAX_NUTRIENTS;
    cell->scent_level = MAX_SCENT;
    curr_food_cells++;
  }

  assign_colour(cell, renderer_state);
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint col, uint row, uint slice) {
  MoldCell *next_cell = (MoldCell *)cell_to_update;
  const MoldCell *grid = (const MoldCell *)curr_sim_state;

  const MoldCell *current_cell =
      get_cell_safe(grid, (int)col, (int)row, (int)slice);
  if (!current_cell)
    return;

  *next_cell = *current_cell;

  update_thresholds();

  float max_neighbour_pheromone = 0;
  float max_neighbour_scent = 0;
  float max_attraction_score = 0;
  bool has_active_neighbour = false;
  // find the neighbours with highest attraction score
  for (int ds = -1; ds <= 1; ds++) {
    for (int dr = -1; dr <= 1; dr++) {
      for (int dc = -1; dc <= 1; dc++) {
        // skip itself
        if (ds == 0 && dr == 0 && dc == 0)
          continue;
        // skip the corners so only nearest neighbours are affected
        if (abs(ds) + abs(dr) + abs(dc) == 3) {
          continue;
        }

        const MoldCell *neighbour =
            get_cell_safe(grid, (int)col + dc, (int)row + dr, (int)slice + ds);

        // find the most attractive neighbour
        if (neighbour) {
          if (neighbour->pheromone_level > max_neighbour_pheromone) {
            max_neighbour_pheromone = neighbour->pheromone_level;
          }

          if (neighbour->scent_level > max_neighbour_scent) {
            max_neighbour_scent = neighbour->scent_level;
          }

          float attraction_score = neighbour->pheromone_level +
                                   (neighbour->scent_level * SCENT_WEIGHT);
          if (attraction_score > max_attraction_score) {
            max_attraction_score = attraction_score;
          }

          if (neighbour->pheromone_level >= dynamic_mold_threshold) {
            has_active_neighbour = true;
          }
        }
      }
    }
  }

  bool food_signaling_cell = false;

  // "latch" onto the food and secrete maximum pheromones
  if (current_cell->pheromone_level < dynamic_mold_threshold &&
      current_cell->nutrient_level > 0 && has_active_neighbour) {
    next_cell->pheromone_level = MAX_PHEROMONE;
    next_cell->scent_level = 0;
    food_signaling_cell = true;
  }

  // use up nutrients to sustain older active mold
  bool is_active_mold = next_cell->pheromone_level >= dynamic_mold_threshold;
  if (is_active_mold && !food_signaling_cell) {

    if (next_cell->nutrient_level > 0) {
      float consumed = (next_cell->nutrient_level >= CONSUMPTION_RATE)
                           ? CONSUMPTION_RATE
                           : next_cell->nutrient_level;
      next_cell->nutrient_level -= consumed;
      next_cell->pheromone_level = MAX_PHEROMONE;
      food_signaling_cell = true;
    }
  }

  if (current_cell->nutrient_level == MAX_NUTRIENTS &&
      (uint)current_cell->pheromone_level == 0) {
    // food continuously emits maximum scent
    next_cell->scent_level = MAX_SCENT;
  } else {
    // diffuse scent rapidly into empty space
    float scent_gradient = max_neighbour_scent - current_cell->scent_level;
    float transported_scent =
        current_cell->scent_level +
        ((scent_gradient * SCENT_DIFFUSION_COEFF) * SCENT_RETENTION_RATE);

    if (transported_scent > dynamic_scent_decay_amount) {
      next_cell->scent_level = transported_scent - dynamic_scent_decay_amount;
    } else {
      next_cell->scent_level = 0;
    }
  }

  // diffuse pheromones and scent from high concentration region
  // to low concentration region
  if (!food_signaling_cell) {
    // find diffusion gradient
    float gradient = max_neighbour_pheromone - current_cell->pheromone_level;
    float pull_strength = PHEROMONE_DIFFUSION_COEFF * PHEROMONE_RETENTION_RATE;

    if (max_neighbour_scent > SCENT_NOISE_FLOOR) {
      // aggressively flow towards the highest attraction core
      pull_strength += (current_cell->scent_level / MAX_SCENT) * SCENT_WEIGHT;
      // prevent pheromone creation from thin air due to high pull strength
      if (pull_strength > 1.0f) {
        pull_strength = 1.0f;
      }
    }

    float transported_pheromone =
        current_cell->pheromone_level + gradient * pull_strength;

    // cap the pheromones to maximum
    if (transported_pheromone > MAX_PHEROMONE) {
      transported_pheromone = MAX_PHEROMONE;
    }

    // decay some of the pheromones
    if (transported_pheromone > dynamic_pheromone_decay_amount) {
      next_cell->pheromone_level =
          (transported_pheromone - dynamic_pheromone_decay_amount);
    } else {
      next_cell->pheromone_level = 0;
    }
  }

  assign_colour(next_cell, renderer_state);
}

static void assign_colour(const MoldCell *cell, RenderInfo *renderer_cell) {
  if (cell->pheromone_level >= dynamic_mold_threshold) {
    // active mold - bright yellow/orange
    uint8_t opacity =
        (uint8_t)((cell->pheromone_level * MAX_RGBA_VAL) / MAX_PHEROMONE);
    *renderer_cell = MOLD_COLOUR(opacity);
  } else if (cell->pheromone_level > dynamic_pheromone_decay_amount &&
             (uint)cell->nutrient_level == 0) {
    // pheromones - transluscent pink
    *renderer_cell = PHEROMONE_COLOUR;
  } else if (cell->nutrient_level > 0) {
    // food - cyan
    *renderer_cell = FOOD_COLOUR;
  } else {
    // dead/empty - invisible
    *renderer_cell = EMPTY_COLOUR;
  }
}

static inline const MoldCell *get_cell_safe(const MoldCell *sim_grid, int col,
                                            int row, int slice) {
  if (col < 0 || col >= (int)grid_width || row < 0 || row >= (int)grid_height ||
      slice < 0 || slice >= (int)grid_depth) {
    return NULL;
  }
  // extracts the cell
  return &sim_grid[INDEX(col, row, slice, grid_width, grid_height)];
}

static void update_thresholds(void) {
  int random_num = rand();
  dynamic_mold_threshold = MOLD_THRESHOLD + (float)((random_num % MOLD_JITTER));
  dynamic_pheromone_decay_amount =
      PHEROMONE_DECAY_AMOUNT + (float)((random_num % PHEROMONE_DECAY_JITTER));
  dynamic_scent_decay_amount =
      SCENT_DECAY_AMOUNT + (float)((random_num % SCENT_DECAY_JITTER));
}
#include "config.h"
#include "extension_types.h"
#include <math.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdlib.h>

#define G 10.0f
#define COEFFICIENT_OF_AIR_RESISTANCE 1.0f
#define DELTA_TIME 0.1f

#define FORCE_CHECK_BEYOND_CELL 3
#define MOVE_CHECK_BEYOND_CELL 1

#define START_PERCENT 40
#define FLOAT_EPSILON 1e-6f
#define MAX_INITIAL_SPEED 1.0f
#define COEFFICIENT_OF_RESTITUTION 0.8f

#define EMPTY_SPACE_COLOUR ((RenderInfo){63, 34, 88, 70})
#define STARS_COLOUR ((RenderInfo){255, 204, 125, 150})

static inline float squared(float x) { return x * x; }

typedef struct {
  float x;
  float y;
  float z;
} Vector;

#define ZERO_VECTOR ((Vector){0.0f, 0.0f, 0.0f})

static inline bool equals(Vector a, Vector b) {
  return fabsf(a.x - b.x) <= FLOAT_EPSILON &&
         fabsf(a.y - b.y) <= FLOAT_EPSILON && fabsf(a.z - b.z) <= FLOAT_EPSILON;
}

static inline Vector minus(Vector a, Vector b) {
  return ((Vector){a.x - b.x, a.y - b.y, a.z - b.z});
}

static inline Vector plus(Vector a, Vector b) {
  return ((Vector){a.x + b.x, a.y + b.y, a.z + b.z});
}

static inline void increment(Vector *a, Vector b) {
  a->x += b.x;
  a->y += b.y;
  a->z += b.z;
}

static inline Vector scale(float a, Vector b) {
  return ((Vector){a * b.x, a * b.y, a * b.z});
}

static inline Vector negate(Vector a) { return ((Vector){-a.x, -a.y, -a.z}); }

static inline float magnitude_squared(Vector a) {
  return squared(a.x) + squared(a.y) + squared(a.z);
}

typedef struct {

  uint mass;

  Vector velocity;

  // precise position beyond which grid cell it's in
  Vector position;

  uint64_t id;

} ParticleCell;

#define AIR_CELL                                                               \
  ((ParticleCell){                                                             \
      .mass = 0, .velocity = ZERO_VECTOR, .position = ZERO_VECTOR, .id = 0})
#define MASS_CELL                                                              \
  ((ParticleCell){                                                             \
      .mass = 1, .velocity = ZERO_VECTOR, .position = ZERO_VECTOR, .id = 0})

const size_t cell_size = sizeof(ParticleCell);

static uint grid_width;
static uint grid_height;
static uint grid_depth;
static uint64_t next_particle_id = 1;
static _Atomic uint64_t *intended_destination_by_id;
static size_t intended_destination_capacity;

static inline float random_unit_float(void) {
  return (float)rand() / (float)RAND_MAX;
}

static inline float random_signed_float(float max_abs) {
  return ((random_unit_float() * 2.0f) - 1.0f) * max_abs;
}

static inline void random_velocity(Vector *velocity) {
  *velocity = (Vector){random_signed_float(MAX_INITIAL_SPEED),
                       random_signed_float(MAX_INITIAL_SPEED),
                       random_signed_float(MAX_INITIAL_SPEED)};
}

static inline bool position_outside_grid(Vector position) {
  return position.x < 0.0f || position.x >= (float)grid_width ||
         position.y < 0.0f || position.y >= (float)grid_height ||
         position.z < 0.0f || position.z >= (float)grid_depth;
}

static inline bool position_outside_cell(Vector position, uint x, uint y,
                                         uint z) {
  float min_x = (float)x;
  float min_y = (float)y;
  float min_z = (float)z;
  float max_x = min_x + 1.0f;
  float max_y = min_y + 1.0f;
  float max_z = min_z + 1.0f;

  return position.x < min_x || position.x >= max_x || position.y < min_y ||
         position.y >= max_y || position.z < min_z || position.z >= max_z;
}

static inline Vector bounce_velocity(Vector velocity) {
  return scale(COEFFICIENT_OF_RESTITUTION, negate(velocity));
}

static inline uint64_t allocate_particle_id(void) { return next_particle_id++; }

static inline uint64_t pack_destination(uint x, uint y, uint z) {
  return (uint64_t)INDEX(x, y, z, grid_width, grid_height) + 1u;
}

static inline bool unpack_destination(uint64_t packed, uint *x, uint *y,
                                      uint *z) {
  if (packed == 0) {
    return false;
  }

  uint64_t index = packed - 1u;
  uint64_t layer = (uint64_t)grid_width * (uint64_t)grid_height;
  *z = (uint)(index / layer);
  index -= (uint64_t)(*z) * layer;
  *y = (uint)(index / (uint64_t)grid_width);
  *x = (uint)(index % (uint64_t)grid_width);
  return true;
}

static inline void set_intended_destination(uint64_t id, uint64_t packed) {
  if (id < intended_destination_capacity) {
    atomic_store_explicit(&intended_destination_by_id[id], packed,
                          memory_order_relaxed);
  }
}

static inline uint64_t get_intended_destination(uint64_t id) {
  if (id < intended_destination_capacity) {
    return atomic_load_explicit(&intended_destination_by_id[id],
                                memory_order_relaxed);
  }
  return 0;
}

void dims_init(uint width, uint height, uint depth) {
  grid_width = width;
  grid_height = height;
  grid_depth = depth;
  next_particle_id = 1;
  intended_destination_capacity =
      (size_t)width * (size_t)height * (size_t)depth + 1u;
  free((void *)intended_destination_by_id);
  intended_destination_by_id = calloc(intended_destination_capacity,
                                      sizeof(*intended_destination_by_id));
}

ParticleCell get_cell(const ParticleCell *grid, int x, int y, int z) {
  if (x < 0 || x >= (int)grid_width || y < 0 || y >= (int)grid_height ||
      z < 0 || z >= (int)grid_depth) {
    return AIR_CELL;
  }

  return grid[INDEX((uint)x, (uint)y, (uint)z, grid_width, grid_height)];
}

static Vector force_between_cells(ParticleCell cell1, ParticleCell cell2) {

  if (cell1.id == cell2.id) {
    return ZERO_VECTOR;
  }

  Vector difference = minus(cell2.position, cell1.position);

  if (equals(difference, ZERO_VECTOR)) {
    return ZERO_VECTOR;
  }

  float distance_squared = magnitude_squared(difference);

  // using the law of universal gravitation, and converting difference to a unit
  // vector at the same time
  return scale((float)(G * (float)cell1.mass * (float)cell2.mass /
                       powf(distance_squared, 1.5f)),
               difference);
}

static void update_velocity(ParticleCell *cell_to_update,
                            const ParticleCell *grid, ParticleCell current,
                            uint x, uint y, uint z) {

  Vector force = ZERO_VECTOR;

  for (int dz = -FORCE_CHECK_BEYOND_CELL; dz <= FORCE_CHECK_BEYOND_CELL; dz++) {
    for (int dy = -FORCE_CHECK_BEYOND_CELL; dy <= FORCE_CHECK_BEYOND_CELL;
         dy++) {
      for (int dx = -FORCE_CHECK_BEYOND_CELL; dx <= FORCE_CHECK_BEYOND_CELL;
           dx++) {
        int new_x = (int)x + dx;
        int new_y = (int)y + dy;
        int new_z = (int)z + dz;

        ParticleCell cell2 = get_cell(grid, new_x, new_y, new_z);

        if (cell2.id == current.id) {
          continue;
        }

        Vector contribution = force_between_cells(current, cell2);

        increment(&force, contribution);
      }
    }
  }

  // air resistance, proportional to velocity squared
  {
    float speed = sqrtf(magnitude_squared(current.velocity));
    Vector drag =
        scale(-COEFFICIENT_OF_AIR_RESISTANCE * speed, current.velocity);

    increment(&force, drag);
  }

  Vector acceleration = scale(1.0f / (float)current.mass, force);

  cell_to_update->velocity =
      plus(current.velocity, scale(DELTA_TIME, acceleration));
}

static void update_position(ParticleCell *cell_to_update,
                            const ParticleCell *grid, ParticleCell current) {
  Vector next_position =
      plus(current.position, scale(DELTA_TIME, current.velocity));

  // check for collision with outside grid
  if (position_outside_grid(next_position)) {
    *cell_to_update = AIR_CELL;
    return;
  }

  uint next_x = (uint)next_position.x;
  uint next_y = (uint)next_position.y;
  uint next_z = (uint)next_position.z;
  uint64_t packed_destination = pack_destination(next_x, next_y, next_z);

  set_intended_destination(current.id, packed_destination);

  for (uint64_t other_id = 1; other_id < intended_destination_capacity;
       other_id++) {
    if (other_id == current.id) {
      continue;
    }

    if (get_intended_destination(other_id) == packed_destination) {
      cell_to_update->position = current.position;
      cell_to_update->velocity = bounce_velocity(current.velocity);
      cell_to_update->mass = current.mass;
      cell_to_update->id = current.id;
      return;
    }
  }

  ParticleCell occupant = get_cell(grid, (int)next_x, (int)next_y, (int)next_z);

  if (occupant.mass > 0 && occupant.id != current.id) {
    cell_to_update->position = current.position;
    cell_to_update->velocity = bounce_velocity(current.velocity);
    cell_to_update->mass = current.mass;
    cell_to_update->id = current.id;
    return;
  }

  cell_to_update->position = next_position;
}

static void assign_colour(RenderInfo *renderer_state, ParticleCell cell) {
  if (cell.mass > 0) {
    *renderer_state = STARS_COLOUR;
  } else {
    *renderer_state = EMPTY_SPACE_COLOUR;
  }
}

void cell_init(void *cell_to_update, RenderInfo *renderer_state, uint col,
               uint row, uint slice) {
  (void)col;
  (void)row;
  (void)slice;

  ParticleCell *cell = cell_to_update;

  if ((rand() % 100) < START_PERCENT) {
    *cell = MASS_CELL;
    cell->mass = 1u;
    cell->id = allocate_particle_id();
    random_velocity(&cell->velocity);

    cell->position =
        (Vector){(float)col + 0.5f, (float)row + 0.5f, (float)slice + 0.5f};

  } else {
    *cell = AIR_CELL;
  }

  assign_colour(renderer_state, *cell);
}

void cell_update(void *cell_to_update, RenderInfo *renderer_state,
                 const void *curr_sim_state, uint x, uint y, uint z) {

  ParticleCell *next = cell_to_update;
  const ParticleCell *grid = curr_sim_state;

  ParticleCell current = get_cell(grid, (int)x, (int)y, (int)z);

  if (position_outside_cell(current.position, x, y, z)) {
    current = AIR_CELL;
  }

  for (int dz = -MOVE_CHECK_BEYOND_CELL; dz <= MOVE_CHECK_BEYOND_CELL; dz++) {
    for (int dy = -MOVE_CHECK_BEYOND_CELL; dy <= MOVE_CHECK_BEYOND_CELL; dy++) {
      for (int dx = -MOVE_CHECK_BEYOND_CELL; dx <= MOVE_CHECK_BEYOND_CELL;
           dx++) {
        int new_x = (int)x + dx;
        int new_y = (int)y + dy;
        int new_z = (int)z + dz;

        ParticleCell cell2 = get_cell(grid, new_x, new_y, new_z);

        if (!position_outside_cell(cell2.position, x, y, z)) {
          current = cell2;
        }
      }
    }
  }

  if (current.mass == 0) {
    *next = AIR_CELL;

    assign_colour(renderer_state, *next);
    return;
  }

  ParticleCell updated = current;
  update_velocity(&updated, grid, current, x, y, z);
  update_position(&updated, grid, current);
  updated.id = current.id;
  *next = updated;

  assign_colour(renderer_state, *next);
}

#include "frustum.h"
#include <cglm/vec3.h>

// Names of the compute shader's uniform variables
#define LEFT_PLANE_UNIFORM "uLeft"
#define RIGHT_PLANE_UNIFORM "uRight"
#define TOP_PLANE_UNIFORM "uTop"
#define BOTTOM_PLANE_UNIFORM "uBottom"
#define NEAR_PLANE_UNIFORM "uNear"
#define FAR_PLANE_UNIFORM "uFar"
#define VIEW_DIR_UNIFORM "uViewDir"

// Location identifiers of the compute shader's uniform variables
static GLint left_plane_loc;
static GLint right_plane_loc;
static GLint top_plane_loc;
static GLint bottom_plane_loc;
static GLint near_plane_loc;
static GLint far_plane_loc;
static GLint view_dir_loc;
static bool opacity;

// Adds (negate is false) or subtracts (negate is true) 2 rows of a matrix,
// normalises and writes the result to the destination uniform variable
static void sum_matrix_rows_to(GLint dest, mat4 mat, int row_a, int row_b,
                               bool negate);

// Divide all 4 components by the square root of the squares of the
// first 3 components in place
static void normalise_row(vec4 row);

void frustum_init(GLuint compute_shader, bool opacity_flag) {
  left_plane_loc = glGetUniformLocation(compute_shader, LEFT_PLANE_UNIFORM);
  right_plane_loc = glGetUniformLocation(compute_shader, RIGHT_PLANE_UNIFORM);
  top_plane_loc = glGetUniformLocation(compute_shader, TOP_PLANE_UNIFORM);
  bottom_plane_loc = glGetUniformLocation(compute_shader, BOTTOM_PLANE_UNIFORM);
  near_plane_loc = glGetUniformLocation(compute_shader, NEAR_PLANE_UNIFORM);
  far_plane_loc = glGetUniformLocation(compute_shader, FAR_PLANE_UNIFORM);
  view_dir_loc = glGetUniformLocation(compute_shader, VIEW_DIR_UNIFORM);
  const GLint opacity_loc =
      glGetUniformLocation(compute_shader, OPACITY_UNIFORM);
  glUseProgram(compute_shader);
  glUniform1i(opacity_loc, opacity_flag);
  opacity = opacity_flag;
}

// The standard method (Gribb & Hartmann, 2001) derives all 6 planes
// from the combined VP matrix by adding/subtracting its rows.
// Each plane is a vec4 (a, b, c, d) where ax + by + cz + d = 0
void frustum_extract(mat4 vp, vec3 view_pos) {
  sum_matrix_rows_to(left_plane_loc, vp, 3, 0, false);
  sum_matrix_rows_to(right_plane_loc, vp, 3, 0, true);
  sum_matrix_rows_to(top_plane_loc, vp, 3, 1, false);
  sum_matrix_rows_to(bottom_plane_loc, vp, 3, 1, true);
  sum_matrix_rows_to(near_plane_loc, vp, 3, 2, false);
  sum_matrix_rows_to(far_plane_loc, vp, 3, 2, true);

  if (opacity) {
    vec3 view_dir;
    glm_vec3_negate_to(view_pos, view_dir);
    glm_normalize(view_dir);
    glUniform3fv(view_dir_loc, 1, view_dir);
  }
}

inline void sum_matrix_rows_to(GLint dest, mat4 mat, int row_a, int row_b,
                               bool negate) {
  vec4 sum;

  // cglm is column-major: vp[col][row]
  for (int i = 0; i < 4; i++) {
    const float operand_2 = mat[i][row_b];
    sum[i] = mat[i][row_a] + ((negate) ? -operand_2 : operand_2);
  }

  normalise_row(sum);
  glUniform4fv(dest, 1, sum);
}

inline void normalise_row(vec4 row) {
  float divisor = 0;

  for (int i = 0; i < 3; i++) {
    const float entry = row[i];
    divisor += entry * entry;
  }

  divisor = sqrtf(divisor);

  for (int i = 0; i < 4; i++) {
    row[i] /= divisor;
  }
}
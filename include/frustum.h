#ifndef FRUSTUM_H
#define FRUSTUM_H

#include <cglm/cglm.h>
#include <glad/glad.h>

// This module performs the frustum culling optimisation.
// The view frustum is the 6-sided volume the camera can see.
// Any cell outside it gets clipped by the GPU anyway, but we skip rendering
// them in the first place.

// Sets up connections to the frustum culling compute shader's uniform
// variables. MUST be called before frustum_extract
// Side effect: makes compute_shader the active GPU programme.
extern void frustum_init(GLuint compute_shader, bool opacity_flag);

// Calculates a view frustum using the combined view and projection matrices
// and writes it to the compute shader's uniform variables.
// The frustum culler MUST be the active GPU programme.
extern void frustum_extract(mat4 vp, vec3 view_pos);

#endif
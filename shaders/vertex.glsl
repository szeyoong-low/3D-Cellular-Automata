// Runs once per vertex to decide its position
#version 450 core

#define POSITION_ATTR_BINDING 0
#define NORMAL_ATTR_BINDING 1
#define OFFSET_ATTR_BINDING 2
#define COLOUR_ATTR_BINDING 3

#define DIMENSION_SCALE 0.5 // Multiply by 0.5 to produce unit cubes
#define DEFAULT_W_COMPONENT 1.0

#define X_AXIS_POSITIVE vec3(1, 0, 0)
#define X_AXIS_NEGATIVE vec3(-1, 0, 0)
#define Y_AXIS_POSITIVE vec3(0, 1, 0)
#define Y_AXIS_NEGATIVE vec3(0, -1, 0)
#define Z_AXIS_POSITIVE vec3(0, 0, 1)
#define Z_AXIS_NEGATIVE vec3(0, 0, -1)
#define FACES_PER_CUBE 6

// Order: front -> back -> left -> right -> bottom -> top
// In OpenGL, the y-axis grows upwards, x-axis righwards, and z-axis towards you
// Tangent × bitangent must equal the outward normal so that transformed
// vertices wind counter-clockwise.

// Where local x-axis of the face maps to
const vec3 TANGENTS[FACES_PER_CUBE] =
    vec3[FACES_PER_CUBE](X_AXIS_POSITIVE, X_AXIS_NEGATIVE, Z_AXIS_POSITIVE,
                         Z_AXIS_NEGATIVE, X_AXIS_POSITIVE, X_AXIS_POSITIVE);

// Where local y-axis of the face maps to
const vec3 BITANGENTS[FACES_PER_CUBE] =
    vec3[FACES_PER_CUBE](Y_AXIS_POSITIVE, Y_AXIS_POSITIVE, Y_AXIS_POSITIVE,
                         Y_AXIS_POSITIVE, Z_AXIS_POSITIVE, Z_AXIS_NEGATIVE);

// Local offset (to set up faces within cube)
const vec3 NORMALS[FACES_PER_CUBE] =
    vec3[FACES_PER_CUBE](Z_AXIS_POSITIVE, Z_AXIS_NEGATIVE, X_AXIS_NEGATIVE,
                         X_AXIS_POSITIVE, Y_AXIS_NEGATIVE, Y_AXIS_POSITIVE);

// Uploaded from C via glUniformMatrix4fv each frame
// No need for a model matrix (local space → world space) - done by instance
// offset
// View: world space → camera space
// Project: camera space → clip space (applies perspective)
uniform mat4 uViewProj; // View composed with project

// Declaration of input variable aPos (per-vertex position attribute)
// in: from CPU side (VAO)
// vec3: 3-component float vector
// location 0 matches attribute slot 0 in the VAO
layout(location = POSITION_ATTR_BINDING) in vec3 aPos;

// Per-instance world position, fed from the instance VBO
layout(location = OFFSET_ATTR_BINDING) in vec3 aOffset;

// Calculating transformed normals, for Phong lighting
layout(location = NORMAL_ATTR_BINDING) in uint aNormalIndex;

out flat vec3 vNormal;  // local normal transformed into world space
out flat vec3 vFragPos; // World space position of the vertex

// Sent to fragment shader
layout(location = COLOUR_ATTR_BINDING) in vec4 aColor; // Already normalised

out flat vec4 vColor;

// A uniform is a variable set from the CPU (your C code) that stays constant
// for every vertex and fragment in a single draw call. Contrast it with in
// variables, which change per vertex.

// The first 3 dimensions represent rotations and scales (matrix
// multiplication). The 4th dimension is needed to represent translation
// (addition). With that, a translation can be written as a 4×4 matrix multiply:
//   [1  0  0  tx]   [x]   [x + tx]
//   [0  1  0  ty] × [y] = [y + ty]
//   [0  0  1  tz]   [z]   [z + tz]
//   [0  0  0   1]   [1]   [  1   ]

// The projection matrix also uses w differently: after multiplying by uProj,
// w is no longer 1. The GPU then automatically divides xyz by w (the
// perspective divide), which is what makes far-away things appear smaller.
// Setting w = 1.0 in vec4(aPos, 1.0) is what marks a vertex as a point that
// should be translated and perspective-divided.

void main() {
  // Matrix multiplication applied right-to-left
  // aOffset shifts the entire cube to its world position
  // aPos gives the position of each corner
  // Multiply by 0.5 to produce unit cubes
  vec4 worldSpacePos =
      vec4(aPos * DIMENSION_SCALE + aOffset, DEFAULT_W_COMPONENT);

  // gl_Position is a variable that must be written to
  gl_Position = uViewProj * worldSpacePos;

  // vFragPos stays in world space as it's what fragment shader needs
  vFragPos = vec3(worldSpacePos);
  vNormal = NORMALS[aNormalIndex];

  vColor = aColor;
}

#include "mesh.h"

const int8_t cube_vertices[UNIQUE_VERTEX_COUNT * CUBE_COMPONENTS_PER_VERTEX] = {
    // Front face (z = +1) — viewed from +z
    -1, // bottom-left
    -1,
    1,
    0,
    1, // bottom-right
    -1,
    1,
    0,
    1, // top-right
    1,
    1,
    0,
    -1, // top-left
    1,
    1,
    0,

    // Back face (z = -1) — viewed from -z; left/right mirrored vs. front
    1,
    -1,
    -1,
    1,
    -1,
    -1,
    -1,
    1,
    -1,
    1,
    -1,
    1,
    1,
    1,
    -1,
    1,

    // Left face (x = -1) — viewed from -x; left/right mirrored in z
    -1,
    -1,
    -1,
    2,
    -1,
    -1,
    1,
    2,
    -1,
    1,
    1,
    2,
    -1,
    1,
    -1,
    2,

    // Right face (x = +1) — viewed from +x
    1,
    -1,
    1,
    3,
    1,
    -1,
    -1,
    3,
    1,
    1,
    -1,
    3,
    1,
    1,
    1,
    3,

    // Bottom face (y = -1) — viewed from -y
    -1,
    -1,
    -1,
    4,
    1,
    -1,
    -1,
    4,
    1,
    -1,
    1,
    4,
    -1,
    -1,
    1,
    4,

    // Top face (y = +1) — viewed from +y
    -1,
    1,
    1,
    5,
    1,
    1,
    1,
    5,
    1,
    1,
    -1,
    5,
    -1,
    1,
    -1,
    5,
};

// Each face's vertices are listed CCW when viewed from outside; verified by
// checking that (v1-v0) × (v2-v0) points outward for every triangle.
const uint8_t cube_indices[CUBE_VERTEX_COUNT] = {
    0, // Front face
    1,  2,  0,  2,  3,

    4, // Back face
    5,  6,  4,  6,  7,

    8, // Left face
    9,  10, 8,  10, 11,

    12, // Right face
    13, 14, 12, 14, 15,

    16, // Bottom face
    17, 18, 16, 18, 19,

    20, // Top face
    21, 22, 20, 22, 23,
};
#include "face.h"

const int8_t face_vertices[FACE_UNIQUE_VERTICES * FACE_COMPONENTS_PER_VERTEX] =
    {
        -1, -1, // Bottom left corner
        1,  -1, // Bottom right corner
        1,  1,  // Top right corner
        -1, 1,  // Top left corner
};

// Counter-clockwise winding
const uint8_t face_indices[FACE_TOTAL_VERTICES] = {
    0, 1, 2, 0, 2, 3,
};
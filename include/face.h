#ifndef FACE_H
#define FACE_H

#include <inttypes.h>

#define FACES_PER_CUBE 6
#define FACE_TOTAL_VERTICES 6
#define FACE_UNIQUE_VERTICES 4
// Will be placed in 3D space by vertex shader
#define FACE_COMPONENTS_PER_VERTEX 2

extern const int8_t
    face_vertices[FACE_UNIQUE_VERTICES * FACE_COMPONENTS_PER_VERTEX];

extern const uint8_t face_indices[FACE_TOTAL_VERTICES];

#endif
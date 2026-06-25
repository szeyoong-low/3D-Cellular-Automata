#ifndef MESH_H
#define MESH_H

#include <inttypes.h>

// Defines the mesh of a single cube.
// Vertices consist of positions and normals

// In OpenGL, a vertex is a bundle of all its attributes — position, surface
// normal, texture coordinate, etc. The GPU processes one bundle per vertex.

// A cube corner sits at the junction of 3 faces that point in different
// directions. Because the normal may differs, that corner must appear as 3
// separate vertex records.
// 8 corners x 3 vertices = 24 vertices
#define UNIQUE_VERTEX_COUNT 24
// This counts the number of vertex references needed.
// 6 faces x 2 triangles x 3 vertices = 36 vertices
#define CUBE_VERTEX_COUNT 36
#define POSITION_NUM_COMPONENTS 3

// (x, y, z, ni)
// first 3 are Cartesian coordinates for position, last 1 is an identifier for
// one of 6 normal vectors
#define CUBE_COMPONENTS_PER_VERTEX 4

// Cube centred at the origin, side length 2.
extern const int8_t
    cube_vertices[UNIQUE_VERTEX_COUNT * CUBE_COMPONENTS_PER_VERTEX];

// When the GPU receives 3 vertices for a triangle, it needs to know which side
// is the "front" (the side facing outward) and which is the "back" (facing
// inward). The convention is to use the order the vertices are listed:
// front faces are counter-clockwise when viewed from outside.
// This is useful for OpenGL's back-face culling optimisation

// To eliminate duplicate vertex listings, we list only unique vertices above
// (stored in a VBO) and reference their indices when defining the triangles
// (stored in an EBO)
extern const uint8_t cube_indices[CUBE_VERTEX_COUNT];

#endif

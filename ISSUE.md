# Refine cube-level culling to face-level culling

Perf: face-level culling

## Issues addressed
1. [Phong lighting needs to be applied over entire clusters of cells](https://github.com/szeyoong-low/3D-Cellular-Automata/issues/1)
4. [Interior face culling](https://github.com/szeyoong-low/3D-Cellular-Automata/issues/4)

## Context
The current stream compaction pipeline in [`hidden.glsl`](/shaders/hidden.glsl) culls (by creating a buffer of hidden cells):
- Cubes below the opacity floor (considered transparent)
- Cubes whose neighbours are all above the opacity ceiling (considered opaque), unless they are at the edge of the simulation grid (subject to the previous cull)

## Changes
The buffer produced by this compute shader should contain a 6-bit mask per cell indicating which of its 6 faces are exposed, instead of 1 for each cube. We have 3 possibilities, subject to the existing logic:
- The surface area of a cluster of cubes and faces at the boundary of the grid should be rendered unconditionally
- A boundary between two cubes of the same packed colour should be culled unconditionally
- A boundary between two cubes of different packed colours should be rendered if and only if the other cube is below the opacity ceiling. If both cells are below the opacity ceiling, the back face will be culled automatically.

Instead of one instance per cube, the frustum cull compute shader will output one instance per visible face for each visible cube (mask is not zero, passes frustum test). Each instance carries its offset, color, and which face it represents (so you index into NORMALS[6]). Loop over the 6 bits of the face mask and do one atomicAdd per visible face.

The cube mesh should be replaced with a face mesh (a single quad, expressed as 4 2D vertices and 6 indices), to be oriented by the vertex shader using the face index from the instance data. The placement of faces uses the formula `position = x * tangent + y * bitangent + normal`, where the tangent is the direction the x-coordinate should be mapped to, and likewise for the bitangent and the y-coordinate.

The fragment shader then applies Phong lighting on all faces that make it (the received normal should be a flat).

Buffers will need to be enlarged accordingly.
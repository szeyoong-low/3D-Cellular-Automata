#version 450 core

layout(local_size_x = 16, local_size_y = 8, local_size_z = 8) in;
layout(std430, binding = 0) buffer RenderInfo { uint renderInfo[]; };
layout(std430, binding = 1) buffer HiddenCells { uint hiddenCells[]; };

// x is column, y is row, z is slice, w is uWidth, h is uHeight
#define INDEX(x, y, z, w, h) ((x) + (w) * ((y) + (h) * (z)))
// Each uint[] holds all 4 RGBA channels packed into one 32-bit word
#define ALPHA(index) ((renderInfo[index] >> 24) & 0xFFu)

uniform uint uWidth;
uniform uint uHeight;
uniform uint uDepth;

const uint opacityFloor = 20;
const uint opacityCeiling = 200;

void main() {
  const uint x = gl_GlobalInvocationID.x;
  const uint y = gl_GlobalInvocationID.y;
  const uint z = gl_GlobalInvocationID.z;

  // Out of bounds
  if (x >= uWidth || y >= uHeight || z >= uDepth) {
    return;
  }

  uint hidden = 0;

  if (ALPHA(INDEX(x, y, z, uWidth, uHeight)) < opacityFloor) {
    // Cell itself is considered hidden
    hidden = 1;
  } else if (x == 0 || x == uWidth - 1 || y == 0 || y == uHeight - 1 ||
             z == 0 || z == uDepth - 1) {
    // Cell is at the surface of the entire grid
    hidden = 0;
  } else if (ALPHA(INDEX(x - 1, y, z, uWidth, uHeight)) > opacityCeiling &&
             ALPHA(INDEX(x + 1, y, z, uWidth, uHeight)) > opacityCeiling &&
             ALPHA(INDEX(x, y - 1, z, uWidth, uHeight)) > opacityCeiling &&
             ALPHA(INDEX(x, y + 1, z, uWidth, uHeight)) > opacityCeiling &&
             ALPHA(INDEX(x, y, z - 1, uWidth, uHeight)) > opacityCeiling &&
             ALPHA(INDEX(x, y, z + 1, uWidth, uHeight)) > opacityCeiling) {
    // All its neighbours are opaque
    hidden = 1;
  }

  hiddenCells[INDEX(x, y, z, uWidth, uHeight)] = hidden;
}
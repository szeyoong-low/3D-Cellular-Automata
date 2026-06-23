#version 450 core

layout(local_size_x = 10, local_size_y = 10, local_size_z = 10) in;
layout(std430, binding = 0) buffer RenderInfo { uint renderInfo[]; };
// This buffer contains a 6-bit mask per cell (stored in a uint) indicating
// which of its 6 faces are hidden (corresponding bit set to 1).
layout(std430, binding = 1) buffer HiddenCells { uint hiddenCells[]; };

#define HIDDEN_CUBE 0x3F
#define FULL_CUBE 0x0
#define FRONT_FACE_OFFSET 0  // (z = +1)
#define BACK_FACE_OFFSET 1   // (z = -1)
#define LEFT_FACE_OFFSET 2   // (x = -1)
#define RIGHT_FACE_OFFSET 3  // (x = +1)
#define BOTTOM_FACE_OFFSET 4 // (y = -1)
#define TOP_FACE_OFFSET 5    // (y = +1)

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

  const uint selfIndex = INDEX(x, y, z, uWidth, uHeight);
  const uint selfColour = ALPHA(selfIndex);
  uint hidden = FULL_CUBE;

  if (selfColour < opacityFloor) {
    // Cells below the opacity floor are considered transparent, so all faces
    // are hidden (the whole cell is culled)
    hidden = HIDDEN_CUBE;
  } else if (x == 0 || x == uWidth - 1 || y == 0 || y == uHeight - 1 ||
             z == 0 || z == uDepth - 1) {
    // Visible cells at the edge of the simulation grid are never culled
    hidden = FULL_CUBE;
  } else {
    hidden |= uint(ALPHA(INDEX(x, y, z + 1, uWidth, uHeight)) > opacityCeiling) << FRONT_FACE_OFFSET;
    hidden |= uint(ALPHA(INDEX(x, y, z - 1, uWidth, uHeight)) > opacityCeiling) << BACK_FACE_OFFSET;
    hidden |= uint(ALPHA(INDEX(x - 1, y, z, uWidth, uHeight)) > opacityCeiling) << LEFT_FACE_OFFSET;
    hidden |= uint(ALPHA(INDEX(x + 1, y, z, uWidth, uHeight)) > opacityCeiling) << RIGHT_FACE_OFFSET;
    hidden |= uint(ALPHA(INDEX(x, y - 1, z, uWidth, uHeight)) > opacityCeiling) << BOTTOM_FACE_OFFSET;
    hidden |= uint(ALPHA(INDEX(x, y + 1, z, uWidth, uHeight)) > opacityCeiling) << TOP_FACE_OFFSET;
  }

  hiddenCells[selfIndex] = hidden;
}
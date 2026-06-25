#version 450 core

#define RENDER_INFO_SSBO_BINDING 0
#define HIDDEN_CELL_SSBO_BINDING 1

#define CULLING_LOCAL_SIZE_X 10
#define CULLING_LOCAL_SIZE_Y 10
#define CULLING_LOCAL_SIZE_Z 10

// x is column, y is row, z is slice, w is uWidth, h is uHeight
#define INDEX(x, y, z, w, h) ((x) + (w) * ((y) + (h) * (z)))

// Each uint[] holds all 4 RGBA channels packed into one 32-bit word
#define ALPHA_OFFSET 24
#define ONE_BYTE_MASK 0xFFu
#define ALPHA(index) ((renderInfo[index] >> ALPHA_OFFSET) & ONE_BYTE_MASK)

#define OPACITY_FLOOR 20    // Below which, a cell is considered transparent
#define OPACITY_CEILING 200 // Above which, a cell is considered opaque

#define FULLY_VISIBLE_CUBE 0
#define FULLY_HIDDEN_CUBE 1

#define MIN_X 0
#define MAX_X (uWidth - 1)
#define MIN_Y 0
#define MAX_Y (uHeight - 1)
#define MIN_Z 0
#define MAX_Z (uDepth - 1)

uniform uint uWidth;
uniform uint uHeight;
uniform uint uDepth;

layout(local_size_x = CULLING_LOCAL_SIZE_X, local_size_y = CULLING_LOCAL_SIZE_Y,
       local_size_z = CULLING_LOCAL_SIZE_Z) in;
layout(std430, binding = RENDER_INFO_SSBO_BINDING) buffer RenderInfo {
  uint renderInfo[];
};
layout(std430, binding = HIDDEN_CELL_SSBO_BINDING) buffer HiddenCells {
  uint hiddenCells[];
};

void main() {
  const uint x = gl_GlobalInvocationID.x;
  const uint y = gl_GlobalInvocationID.y;
  const uint z = gl_GlobalInvocationID.z;

  // Out of bounds
  if (x >= uWidth || y >= uHeight || z >= uDepth) {
    return;
  }

  uint hidden = FULLY_VISIBLE_CUBE;
  const uint selfIndex = INDEX(x, y, z, uWidth, uHeight);

  if (ALPHA(selfIndex) < OPACITY_FLOOR) {
    // Cell itself is considered hidden
    hidden = FULLY_HIDDEN_CUBE;
  } else if (x == MIN_X || x == MAX_X || y == MIN_Y || y == MAX_Y ||
             z == MIN_Z || z == MAX_Z) {
    // Cell is at the surface of the entire grid
    hidden = FULLY_VISIBLE_CUBE;
  } else if (ALPHA(INDEX(x - 1, y, z, uWidth, uHeight)) > OPACITY_CEILING &&
             ALPHA(INDEX(x + 1, y, z, uWidth, uHeight)) > OPACITY_CEILING &&
             ALPHA(INDEX(x, y - 1, z, uWidth, uHeight)) > OPACITY_CEILING &&
             ALPHA(INDEX(x, y + 1, z, uWidth, uHeight)) > OPACITY_CEILING &&
             ALPHA(INDEX(x, y, z - 1, uWidth, uHeight)) > OPACITY_CEILING &&
             ALPHA(INDEX(x, y, z + 1, uWidth, uHeight)) > OPACITY_CEILING) {
    // All its neighbours are opaque
    hidden = FULLY_HIDDEN_CUBE;
  }

  hiddenCells[selfIndex] = hidden;
}
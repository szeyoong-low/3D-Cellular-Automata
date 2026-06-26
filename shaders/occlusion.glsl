#version 450 core

#define RENDER_INFO_SSBO_BINDING 0
#define OCCLUSION_SSBO_BINDING 1

#define CULLING_LOCAL_SIZE_X 16
#define CULLING_LOCAL_SIZE_Y 8
#define CULLING_LOCAL_SIZE_Z 8

// x is column, y is row, z is slice, w is uWidth, h is uHeight
#define INDEX(x, y, z, w, h) ((x) + (w) * ((y) + (h) * (z)))

// Each uint[] holds all 4 RGBA channels packed into one 32-bit word
#define ALPHA_OFFSET 24
#define ONE_BYTE_MASK 0xFFu
#define ALPHA(index) ((renderInfo[index] >> ALPHA_OFFSET) & ONE_BYTE_MASK)

#define OPACITY_FLOOR 20    // Below which, a cell is considered transparent
#define OPACITY_CEILING 200 // Above which, a cell is considered opaque

#define FULLY_VISIBLE_CUBE 0x0
#define FULLY_OCCLUDED_CUBE 0x3F
#define FRONT_FACE_OFFSET 0  // (z = +1)
#define BACK_FACE_OFFSET 1   // (z = -1)
#define LEFT_FACE_OFFSET 2   // (x = -1)
#define RIGHT_FACE_OFFSET 3  // (x = +1)
#define BOTTOM_FACE_OFFSET 4 // (y = -1)
#define TOP_FACE_OFFSET 5    // (y = +1)

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
// This buffer contains a 6-bit mask per cell (stored in a uint) indicating
// which of its 6 faces are hidden (corresponding bit set to 1).
layout(std430, binding = OCCLUSION_SSBO_BINDING) buffer OccludedCells {
  uint occludedCells[];
};

void main() {
  const uint x = gl_GlobalInvocationID.x;
  const uint y = gl_GlobalInvocationID.y;
  const uint z = gl_GlobalInvocationID.z;

  // Out of bounds
  if (x >= uWidth || y >= uHeight || z >= uDepth) {
    return;
  }

  uint occluded = FULLY_VISIBLE_CUBE;
  const uint selfIndex = INDEX(x, y, z, uWidth, uHeight);
  const uint selfColour = ALPHA(selfIndex);

  if (selfColour < OPACITY_FLOOR) {
    // Cells below the opacity floor are considered transparent, so all faces
    // are hidden (the whole cell is culled)
    occluded = FULLY_OCCLUDED_CUBE;
  } else if (x == MIN_X || x == MAX_X || y == MIN_Y || y == MAX_Y ||
             z == MIN_Z || z == MAX_Z) {
    // Visible cells at the edge of the simulation grid are never culled
    occluded = FULLY_VISIBLE_CUBE;
  } else {
    occluded |=
        uint(ALPHA(INDEX(x, y, z + 1, uWidth, uHeight)) > OPACITY_CEILING)
        << FRONT_FACE_OFFSET;
    occluded |=
        uint(ALPHA(INDEX(x, y, z - 1, uWidth, uHeight)) > OPACITY_CEILING)
        << BACK_FACE_OFFSET;
    occluded |=
        uint(ALPHA(INDEX(x - 1, y, z, uWidth, uHeight)) > OPACITY_CEILING)
        << LEFT_FACE_OFFSET;
    occluded |=
        uint(ALPHA(INDEX(x + 1, y, z, uWidth, uHeight)) > OPACITY_CEILING)
        << RIGHT_FACE_OFFSET;
    occluded |=
        uint(ALPHA(INDEX(x, y - 1, z, uWidth, uHeight)) > OPACITY_CEILING)
        << BOTTOM_FACE_OFFSET;
    occluded |=
        uint(ALPHA(INDEX(x, y + 1, z, uWidth, uHeight)) > OPACITY_CEILING)
        << TOP_FACE_OFFSET;
  }

  occludedCells[selfIndex] = occluded;
}
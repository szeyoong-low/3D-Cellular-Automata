#version 450 core

#define RENDER_INFO_SSBO_BINDING 0
#define OCCLUSION_SSBO_BINDING 1
#define INSTANCE_SSBO_BINDING 2
#define DRAW_INDIRECT_SSBO_BINDING 3

#define CULLING_LOCAL_SIZE_X 16
#define CULLING_LOCAL_SIZE_Y 8
#define CULLING_LOCAL_SIZE_Z 8

#define INSTANCE_COUNT_INDEX 1

// sqrt(3)/2, the circumradius of a unit cube, the smallest sphere that
// fully encloses it. Using a sphere slightly larger than the cube means
// you'll never accidentally cull a cell that's partially in view.
#define UNIT_CUBE_RADIUS 0.866F

#define FULLY_OCCLUDED_CUBE 0x3F
#define FACES_PER_CUBE 6
#define ONE_BIT_MASK 0x1

// x is column, y is row, z is slice, w is uWidth, h is uHeight
#define INDEX(x, y, z, w, h) ((x) + (w) * ((y) + (h) * (z)))

uniform uint uWidth;
uniform uint uHeight;
uniform uint uDepth;

uniform vec4 uLeft;
uniform vec4 uRight;
uniform vec4 uTop;
uniform vec4 uBottom;
uniform vec4 uNear;
uniform vec4 uFar;
uniform vec3 uViewDir;
uniform bool uOpacity;

struct InstanceData {
  ivec3 offset;
  uint packedColour;
  uint faceIndex;
};

layout(local_size_x = CULLING_LOCAL_SIZE_X, local_size_y = CULLING_LOCAL_SIZE_Y,
       local_size_z = CULLING_LOCAL_SIZE_Z) in;
layout(std430, binding = RENDER_INFO_SSBO_BINDING) buffer RenderInfo {
  uint renderInfo[];
};
layout(std430, binding = OCCLUSION_SSBO_BINDING) buffer OccludedCells {
  uint occludedCells[];
};
layout(std430, binding = INSTANCE_SSBO_BINDING) buffer InstanceBuffer {
  InstanceData instanceBuffer[];
};
layout(std430, binding = DRAW_INDIRECT_SSBO_BINDING) buffer DrawIndirect {
  uint drawIndirect[];
};

// Because a cube isn't a point, we test against a bounding sphere.
bool check_plane(vec4 plane, vec3 center) {
  return dot(plane.xyz, center) + plane.w >= -UNIT_CUBE_RADIUS;
}

// Tests if a sphere lies within the view frustum
void main() {
  const uint x = gl_GlobalInvocationID.x;
  const uint y = gl_GlobalInvocationID.y;
  const uint z = gl_GlobalInvocationID.z;

  // Out of bounds/occluded
  if (x >= uWidth || y >= uHeight || z >= uDepth) {
    return;
  }

  uint occlusionMask = occludedCells[INDEX(x, y, z, uWidth, uHeight)];

  if (occlusionMask == FULLY_OCCLUDED_CUBE) {
    return;
  }

  // Calculate offset
  // Centre the grid at the world origin so the orbit camera target
  // {0,0,0} stays at the grid's centre regardless of grid size
  const int x_off = int(x) - int(uWidth) / 2;
  const int y_off = int(y) - int(uHeight) / 2;
  const int z_off = int(z) - int(uDepth) / 2;
  const vec3 center = vec3(x_off, y_off, z_off);

  if (check_plane(uLeft, center) && check_plane(uRight, center) &&
      check_plane(uTop, center) && check_plane(uBottom, center) &&
      check_plane(uNear, center) && check_plane(uFar, center)) {

    // Generate one instance per visible face.
    for (int i = 0; i < FACES_PER_CUBE; i++, occlusionMask >>= 1) {
      if ((occlusionMask & ONE_BIT_MASK) == 0) {
        uint instance_no = atomicAdd(drawIndirect[INSTANCE_COUNT_INDEX], 1);

        // Prepare the information needed for rendering
        instanceBuffer[instance_no].offset = ivec3(x_off, y_off, z_off);
        instanceBuffer[instance_no].packedColour =
            renderInfo[INDEX(x, y, z, uWidth, uHeight)];
        instanceBuffer[instance_no].faceIndex = i;
      }
    }
  }
}

#version 450 core

// Handles comparison steps where partners are in the same workgroup
// (step size <= workgroup size / 2)
// Load data into shared memory
// Loop through multiple steps with barrier
// Write back to global memory

struct InstanceData {
  ivec3 offset;
  uint packedColour;
};

layout(local_size_x = 1024) in;
layout(std430, binding = 2) buffer InstanceBuffer {
  InstanceData instanceBuffer[];
};
layout(std430, binding = 4) buffer SortKeys { float sortKeys[]; };

uniform uint uBlockSize;
uniform uint uStepSize;

void main() {
  const uint self = gl_GlobalInvocationID.x;
  const uint partner = self ^ uStepSize;

  if (self < partner) {
    const bool ascending = (self & uBlockSize) == 0;

    const float sortKeySelf = sortKeys[self];
    const float sortKeyPartner = sortKeys[partner];

    if (ascending == (sortKeySelf < sortKeyPartner)) {
      sortKeys[self] = sortKeyPartner;
      sortKeys[partner] = sortKeySelf;

      InstanceData temp = instanceBuffer[self];
      instanceBuffer[self] = instanceBuffer[partner];
      instanceBuffer[partner] = temp;
    }
  }
}
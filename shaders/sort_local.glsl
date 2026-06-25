#version 450 core

// Handles comparison steps where partners are in the same workgroup

#define INSTANCE_SSBO_BINDING 2
#define SORT_KEY_SSBO_BINDING 4

#define MIN_STEP 1
#define SORTING_LOCAL_SIZE 1024

uniform uint uBlockSize;
uniform uint uStepSize;

struct InstanceData {
  ivec3 offset;
  uint packedColour;
};

layout(local_size_x = SORTING_LOCAL_SIZE) in;
layout(std430, binding = INSTANCE_SSBO_BINDING) buffer InstanceBuffer {
  InstanceData instanceBuffer[];
};
layout(std430, binding = SORT_KEY_SSBO_BINDING) buffer SortKeys {
  float sortKeys[];
};

shared InstanceData localInstances[SORTING_LOCAL_SIZE];
shared float localKeys[SORTING_LOCAL_SIZE];

void main() {
  // Get each thread to load 1 element from global memory
  const uint selfLocalID = gl_LocalInvocationID.x;
  const uint selfGlobalID = gl_GlobalInvocationID.x;

  localKeys[selfLocalID] = sortKeys[selfGlobalID];
  localInstances[selfLocalID] = instanceBuffer[selfGlobalID];

  barrier();

  // Step size must be a power of two
  for (uint i = uStepSize; i >= MIN_STEP; i /= 2) {
    const uint partnerLocalID = selfLocalID ^ i;

    if (selfLocalID < partnerLocalID) {
      const bool ascending = (selfGlobalID & uBlockSize) == 0;

      const float selfSortKey = localKeys[selfLocalID];
      const float partnerSortKey = localKeys[partnerLocalID];

      if (ascending == (selfSortKey < partnerSortKey)) {
        localKeys[selfLocalID] = partnerSortKey;
        localKeys[partnerLocalID] = selfSortKey;

        InstanceData temp = localInstances[selfLocalID];
        localInstances[selfLocalID] = localInstances[partnerLocalID];
        localInstances[partnerLocalID] = temp;
      }
    }

    barrier();
  }

  // Get each thread to store 1 element to global memory
  sortKeys[selfGlobalID] = localKeys[selfLocalID];
  instanceBuffer[selfGlobalID] = localInstances[selfLocalID];
}
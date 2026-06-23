#version 450 core

#define INSTANCE_SSBO_BINDING 2
#define SORT_KEY_SSBO_BINDING 4

#define SORTING_LOCAL_SIZE_X 256

struct InstanceData {
  ivec3 offset;
  uint packedColour;
  uint faceIndex;
};

// Sorting is 1D work (1 thread/comparison).
// 256 is a good choice as it is a moderately large power of two.
layout(local_size_x = SORTING_LOCAL_SIZE_X) in;
layout(std430, binding = INSTANCE_SSBO_BINDING) buffer InstanceBuffer {
  InstanceData instanceBuffer[];
};
layout(std430, binding = SORT_KEY_SSBO_BINDING) buffer SortKeys { float sortKeys[]; };

uniform uint uBlockSize;
uniform uint uStepSize;

// Bitonic sort: https://www.geeksforgeeks.org/dsa/bitonic-sort/
// Each thread represents one index self. It computes its partner, decides the
// sort direction for its block, and swaps conditionally.
// This is quite similar to mergesort
// Since it is just a fixed sequence of compare-and-swap operations that doesn't
// depend on the data, it is highly efficient as there is no branching and
// is highly parallelisable.

void main() {
  // Division step (controlled by uBlockSize) happens implicitly when we
  // change the sort direction, so the bitonic sequences are built up
  // as we progress through the stages.
  // For example, when uBlockSize is 2, we sort (by merging) each pair of
  // two elements. Adjacent pairs are alternately increasing and decreasing,
  // so we have a bitonic sequence ready for when uBlockSize is 4
  // Each stage s introduces a new merge at distance uBlockSize/2

  const uint self = gl_GlobalInvocationID.x;
  // The XOR flips a single bit controlled by uStepSize (always a power of two).
  // Every element gets exactly one partner, and the pairing is symmetric.
  const uint partner = self ^ uStepSize;

  // Merge step
  if (self < partner) { // Avoid double-swapping
    // Sort ascending in the first half, and descending in the second.
    // This alternating pattern is what makes the sequence bitonic at each
    // stage.
    const bool ascending = (self & uBlockSize) == 0;

    const float sortKeySelf = sortKeys[self];
    const float sortKeyPartner = sortKeys[partner];

    // Since we want descending order (farthest instance first),
    // we treat ascending blocks as descending, and vice versa.
    if (ascending == (sortKeySelf < sortKeyPartner)) {
      // swap both keys and instance data
      sortKeys[self] = sortKeyPartner;
      sortKeys[partner] = sortKeySelf;

      InstanceData temp = instanceBuffer[self];
      instanceBuffer[self] = instanceBuffer[partner];
      instanceBuffer[partner] = temp;
    }
  }
}
#version 450 core

// No inputs. Simply creates a geometry covering the whole viewport so that
// the composite fragment shader can run.
// This is done by drawing an oversized triangle that is clipped to fill the
// viewport.

#define MASK_SECOND_BIT 0x2
#define XY_COORD_SCALE 2.0F
#define XY_COORD_BIAS 1.0F
#define DEFAULT_Z_COMPONENT 0.0F
#define DEFAULT_W_COMPONENT 1.0F

void main() {
  // Since we invoke this 3 times, we get 0-2 for gl_VertexID

  // Step 1: generate (0,0), (2,0), (0,2)
  vec2 xy_coords =
      vec2((gl_VertexID << 1) & MASK_SECOND_BIT, gl_VertexID & MASK_SECOND_BIT);

  // Step 2: generate (-1,-1), (3,-1), (-1,3)
  // Covers clip space (-1,-1) to (1, 1)
  gl_Position = vec4(xy_coords * XY_COORD_SCALE - XY_COORD_BIAS,
                     DEFAULT_Z_COMPONENT, DEFAULT_W_COMPONENT);
}
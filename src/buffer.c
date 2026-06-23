#include "buffer.h"
#include "graphics_utility.h"
#include "mesh.h"

#define POSITION_NUM_COMPONENTS 3

// Vertex buffer object (VBO): a region of GPU memory holding raw vertex bytes
inline void attribute_buffer_init(GLuint *attribute_buffer) {
  glGenVertexArrays(1, attribute_buffer); // Allocate space in VRAM
  glBindVertexArray(*attribute_buffer);   // Makes it the current VAO
}

inline void vertex_buffer_init(GLuint *vertex_buffer) {
  glGenBuffers(1, vertex_buffer);
  // Declare that this buffer holds vertex attribute data
  glBindBuffer(GL_ARRAY_BUFFER, *vertex_buffer);
  // Upload the mesh from CPU RAM into GPU VRAM
  // GL_STATIC_DRAW: hint that the data is uploaded once and drawn many times
  glNamedBufferData(*vertex_buffer, (GLsizeiptr)sizeof(cube_vertices),
                    cube_vertices, GL_STATIC_DRAW);

  // Tell the GPU how to interpret the bytes in the VBO currently bound to
  // GL_ARRAY_BUFFER for attribute slot 0:
  // - 3 bytes in this attribute
  // - no normalisation
  // - stride: # bytes between consecutive vertices
  // - offset: # bytes from the start of each vertex
  // The slot number 0 will match layout(location = 0) in the vertex shader

  // Note that integers are automatically cast to floats on the GPU side

  // Positions
  glVertexAttribPointer(POSITION_ATTR_BINDING, POSITION_NUM_COMPONENTS, GL_BYTE,
                        GL_FALSE, CUBE_COMPONENTS_PER_VERTEX * sizeof(int8_t),
                        (void *)0);
  glEnableVertexAttribArray(POSITION_ATTR_BINDING);

  // Normals
  glVertexAttribIPointer(NORMAL_ATTR_BINDING, sizeof(int8_t), GL_UNSIGNED_BYTE,
                         CUBE_COMPONENTS_PER_VERTEX * sizeof(int8_t),
                         (void *)(POSITION_NUM_COMPONENTS * sizeof(int8_t)));
  glEnableVertexAttribArray(NORMAL_ATTR_BINDING);
}

// Element buffer object (EBO) tells allows triangles in the mesh to reuse
// vertices
inline void element_buffer_init(GLuint *element_buffer) {
  glGenBuffers(1, element_buffer);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, *element_buffer);
  glNamedBufferData(*element_buffer, (GLsizeiptr)sizeof(cube_indices),
                    cube_indices, GL_STATIC_DRAW);
}

// Shader storage buffer objects (SSBO) allow the GPU to write to them.
// We use them here to cache the results of culling hidden cells.
inline void render_info_buffer_init(GLuint *render_info_buffer, size_t size,
                                    const RenderInfo *data) {
  glGenBuffers(1, render_info_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, RENDER_INFO_SSBO_BINDING,
                   *render_info_buffer);
  glNamedBufferData(*render_info_buffer,
                    (GLsizeiptr)(size * sizeof(RenderInfo)), data,
                    GL_DYNAMIC_DRAW);
}

inline void hidden_cell_buffer_init(GLuint *hidden_cell_buffer,
                                    size_t size) {
  glGenBuffers(1, hidden_cell_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, HIDDEN_CELL_SSBO_BINDING,
                   *hidden_cell_buffer);
  // GLSL's smallest integer type is 32 bits wide
  glNamedBufferData(*hidden_cell_buffer,
                    (GLsizeiptr)(size * sizeof(GLuint)), NULL,
                    GL_DYNAMIC_DRAW);
}

inline void instance_buffer_init(GLuint *instance_buffer, size_t size) {
  glGenBuffers(1, instance_buffer);
  // 2 separate binding points are needed:
  // - VAO attribute for access by the vertex shader
  // - SSBO binding point for access by the compute shader
  glBindBuffer(GL_ARRAY_BUFFER, *instance_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, INSTANCE_SSBO_BINDING,
                   *instance_buffer);
  // This allocates and initialises the buffer (NULL to skip initialisation).
  // GL_DYNAMIC_DRAW: hint to the driver that thid data is re-uploaded
  // frequently This will change on every frame when the simulation runs
  glNamedBufferData(*instance_buffer,
                    (GLsizeiptr)(size * sizeof(InstanceData)), NULL,
                    GL_DYNAMIC_DRAW);

  // The VAO makes recordings for the current VBO bound to GL_ARRAY_BUFFER
  // Position offset
  glVertexAttribPointer(OFFSET_ATTR_BINDING, 3, GL_INT, GL_FALSE,
                        sizeof(InstanceData), (void *)0);
  glEnableVertexAttribArray(OFFSET_ATTR_BINDING);
  glVertexAttribDivisor(OFFSET_ATTR_BINDING,
                        1); // Attribute advances once per instance

  // Packed colour
  // Normalise flag: unsigned integer values mapped to [0, 1]
  glVertexAttribPointer(COLOUR_ATTR_BINDING, sizeof(GLuint), GL_UNSIGNED_BYTE,
                        GL_TRUE, sizeof(InstanceData), (void *)sizeof(ivec3));
  glEnableVertexAttribArray(COLOUR_ATTR_BINDING);
  glVertexAttribDivisor(COLOUR_ATTR_BINDING, 1);
}

// Buffer for a single draw indirect command
inline void draw_indirect_buffer_init(GLuint *draw_indirect_buffer) {
  glGenBuffers(1, draw_indirect_buffer);
  glBindBuffer(GL_DRAW_INDIRECT_BUFFER, *draw_indirect_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, DRAW_INDIRECT_SSBO_BINDING,
                   *draw_indirect_buffer);
  glNamedBufferData(*draw_indirect_buffer, sizeof(DrawElementsIndirectCommand),
                    &draw_indirect_cmd, GL_DYNAMIC_DRAW);
}

inline void sort_key_buffer_init(GLuint *sort_key_buffer, size_t size) {
  glGenBuffers(1, sort_key_buffer);
  glBindBufferBase(GL_SHADER_STORAGE_BUFFER, SORT_KEY_SSBO_BINDING,
                   *sort_key_buffer);
  glNamedBufferData(*sort_key_buffer,
                    (GLsizeiptr)(sizeof(float) * next_power_two(size)),
                    NULL, GL_DYNAMIC_DRAW);
}

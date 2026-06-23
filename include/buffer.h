#ifndef BUFFER_H
#define BUFFER_H

#include "extension_types.h"
#include <cglm/types.h>
#include <glad/glad.h>

#define POSITION_ATTR_BINDING 0
#define OFFSET_ATTR_BINDING 1
#define COLOUR_ATTR_BINDING 2
#define FACE_INDEX_ATTR_BINDING 3

#define RENDER_INFO_SSBO_BINDING 0
#define HIDDEN_CELL_SSBO_BINDING 1
#define INSTANCE_SSBO_BINDING 2
#define DRAW_INDIRECT_SSBO_BINDING 3
#define SORT_KEY_SSBO_BINDING 4

typedef struct {
  ivec3 offset;
  uint packedColour;
  uint faceIndex;
} InstanceData;

// Vertex attribute object (VAO): records all attribute bindings made
// while it is bound
// MUST be the FIRST buffer initialised
extern void attribute_buffer_init(GLuint *attribute_buffer);
extern void vertex_buffer_init(GLuint *vertex_buffer);
extern void element_buffer_init(GLuint *element_buffer);
extern void render_info_buffer_init(GLuint *render_info_buffer, size_t size,
                                    const RenderInfo *data);
extern void hidden_cell_buffer_init(GLuint *hidden_cell_buffer, size_t size);
extern void instance_buffer_init(GLuint *instance_buffer, size_t size);
extern void draw_indirect_buffer_init(GLuint *draw_indirect_buffer);
extern void sort_key_buffer_init(GLuint *sort_key_buffer, size_t size);

#endif
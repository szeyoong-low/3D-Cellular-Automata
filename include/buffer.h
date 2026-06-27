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
#define OCCLUSION_SSBO_BINDING 1
#define INSTANCE_SSBO_BINDING 2
#define DRAW_INDIRECT_SSBO_BINDING 3

typedef struct {
  ivec3 offset;
  uint packedColour;
  uint faceIndex;
  // Beware of struct alignment: vec3 has a base alignment of 16 bytes in GLSL
  uint _pad[3];
} InstanceData;

// Vertex attribute object (VAO): records all attribute bindings made
// while it is bound
// MUST be the FIRST buffer initialised
extern void attribute_buffer_init(GLuint *attribute_buffer);
extern void vertex_buffer_init(GLuint *vertex_buffer);
extern void element_buffer_init(GLuint *element_buffer);
extern void render_info_buffer_init(GLuint *render_info_buffer, size_t size,
                                    const RenderInfo *data);
extern void occlusion_buffer_init(GLuint *occlusion_buffer, size_t size);
extern void instance_buffer_init(GLuint *instance_buffer, size_t size);
extern void draw_indirect_buffer_init(GLuint *draw_indirect_buffer);

#endif
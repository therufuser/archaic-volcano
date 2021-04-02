#include "mesh.hpp"

#include <loguru.hpp>

namespace volcano {
  renderer::mesh::mesh(renderer* parent, const graphics::vertex* vertices, int size) {
    LOG_SCOPE_F(MAX, "Create new mesh (size=%d)", size);
    for(int i = 0; i < size; i++) {
      LOG_SCOPE_F(MAX, "Vertex #%d:", i);
      LOG_F(MAX, "X: %f", vertices[i].x);
      LOG_F(MAX, "Y: %f", vertices[i].y);
      LOG_F(MAX, "Z: %f", vertices[i].z);
      LOG_F(MAX, "W: %f", vertices[i].w);
      LOG_F(MAX, "R: %f", vertices[i].r);
      LOG_F(MAX, "G: %f", vertices[i].g);
      LOG_F(MAX, "B: %f", vertices[i].b);
      LOG_F(MAX, "A: %f", vertices[i].a);
    }

    vert_count = size;

    LOG_F(MAX, "sizeof(vertices): %d", sizeof(vertices));
    vbo = parent->create_buffer(vertices, 16 * size * sizeof(*vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    LOG_F(MAX, "Buffer created (vbo.buffer=%d)", vbo.buffer);
  }

  VkBuffer_T* const* renderer::mesh::get_buffer() {
    return &vbo.buffer;
  }

  int renderer::mesh::get_size() {
    return vert_count;
  }
}

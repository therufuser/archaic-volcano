#include "mesh.hpp"

#include <loguru.hpp>

namespace volcano {
  renderer::mesh::mesh(renderer* parent, const float* vertices, int size) {
    LOG_SCOPE_F(MAX, "Create new mesh (size=%d)", size);
    for(int i = 0; i < size; i++) {
      LOG_F(MAX, "Coordinate #%d: %f", i, vertices[i]);
    }

    LOG_F(MAX, "sizeof(vertices): %d", sizeof(vertices));
    vbo = parent->create_buffer(vertices, size * sizeof(*vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
    LOG_F(MAX, "Buffer cretaed (vbo.buffer=%d)", vbo.buffer);
  }

  VkBuffer_T* const* renderer::mesh::get_buffer() {
    return &vbo.buffer;
  }
}

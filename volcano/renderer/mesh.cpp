#include "mesh.hpp"

namespace volcano {
  renderer::mesh::mesh(renderer* parent, const float* vertices, int size) {
    vbo = parent->create_buffer(vertices, size * sizeof(*vertices), VK_BUFFER_USAGE_VERTEX_BUFFER_BIT);
  }

  VkBuffer_T* const* renderer::mesh::get_buffer() {
    return &vbo.buffer;
  }
}

#ifndef MESH_HPP
  #define MESH_HPP

#include "../volcano.hpp"

namespace volcano {
  class renderer::mesh {
    private:
      struct buffer vbo;

    public:
      mesh(renderer* parent, const float* vertices, int size);

      VkBuffer_T* const* get_buffer();
  };
}

#endif

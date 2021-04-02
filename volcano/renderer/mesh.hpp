#ifndef MESH_HPP
  #define MESH_HPP

#include "../volcano.hpp"
#include "../graphics/vertex.hpp"

namespace volcano {
  class renderer::mesh {
    private:
      struct buffer vbo;
      int vert_count;

    public:
      mesh(renderer* parent, const graphics::vertex* vertices, int size);

      VkBuffer_T* const* get_buffer();
      int get_size();
  };
}

#endif

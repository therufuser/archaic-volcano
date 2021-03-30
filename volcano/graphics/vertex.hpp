#ifndef VERTEX_HPP
  #define VERTEX_HPP

namespace volcano {
  namespace graphics {
    struct vertex {
      float x;
      float y;
      float z;
      float w;

      float r;
      float g;
      float b;
      float a;

      float n_x;
      float n_y;
      float n_z;
    };
  }
}

#endif

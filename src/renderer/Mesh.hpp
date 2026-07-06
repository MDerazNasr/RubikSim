#pragma once
#include <cstddef> //std::size_t -> an unsigned integer type for used for sizes
#include <vector> //std::vectorm -> a dynamic array from the C++ standard library

namespace rubiksim {
struct Vertex {
  float position[3]; // 3 floats x,y,z
  float color[3];    // 3 floats: redm green blue
};

class Mesh {
public:
  Mesh(
      const std::vector<Vertex> &vertices, // vertex data copied from CPU to GPU
      const std::vector<unsigned int>
          &indices // indices data copied from CPU to GpU

  )
}
} // namespace rubiksim

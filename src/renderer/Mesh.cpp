#include "renderer/Mesh.hpp"

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <cstddef>

namespace rubiksim {
// defines the constructor declared in the header
// Mesh:: means this function belongs to the Mesh class
Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<unsigned> &indices)
    : /*
       * Below is a member initializer list
       * it initializes member variables before the constructor body runs
       * indices.size() return a std::size_t - an unsigned int used for sizes
       * static_cast<unsigned> explicitly converts it to unsigned int*/
      vertexArray_(0), vertexBuffer_(0), indexBuffer_(0),
      indexCount_(static_cast<unsigned int>(indices.size())) {}

} // namespace rubiksim

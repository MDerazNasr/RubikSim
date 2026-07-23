#pragma once      // header only once per .cpp file that uses it
#include <vector> //std::vector, a dynamic array from the c++ standarrd libray

namespace rubiksim { // puts our names inside the rubiksim namespace
struct Vertex {
  // vertex x,y,z positions
  float x;
  float y;
  float z;

  // vertex red, green, blue values
  float r;
  float g;
  float b;
};

// class creates a custom type. class members are private by default
class Mesh {
  // everything after public can be used by outside code
public:
  Mesh(
      // const means the constructor will not modify the vector
      // & means pass by reference, avoiding a copy
      const std::vector<Vertex> &vertices, const std::<unsigned int> &indices);
  ~Mesh(); // this is a destructor for when a mesh obj is destroyed

  // Disable copy cuz One OpenGL buffer owner should not be copied
  Mesh(const Mesh &) = delete;
  // Disable copy assignment for the same reason
  Mesh &operator=(const Mesh &) = delete;

  Mesh(Mesh &&other) noexcept; // Move constructor. Transfers ownership from
                               // another mesh
  // Move assignment. Replaces this Mesh by taking another Mesh's resources
  Mesh &operator=(Mesh &&other) noexcept;

  // Draws the mesh. const measn this function should not change the Mesh;a C++
  // state
  void draw() const;

  // everything after private can only be used insde Mesh itself
private:
  // internal cleanup helper
  void destroy();
  // openGL VAO handle
  unsigned int vertexArray_;
  // OpenGL VBO handle
  unsigned int vertexBuffer_;
  // OpenGL EBO handle
  unsigned int indexBuffer_;
  // Number of indices to draw
  unsigned int indexCount_;
};
} // namespace rubiksim

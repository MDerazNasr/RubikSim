// tells compiler to include this header once only per complimation unit --
// prevents duplicate class declarations from the same file
#pragma once
// creates named scope
/*
 * tells the compiler that a type named GLFWwindow exists
 * we do this because the header only needs to store a pointer
 * it does not need the full GLFW def yet
 */
struct GLFWwindow; // forward declaration

namespace rubiksim {
// declares a class named Application
class Application {
  // everything after public is accessible from outsde the class
public:
  // declares a constructor
  Application();
  // declares a destrcutor
  // which runs automatically when an obj is destroyed
  // for this project the destrcutor will clean up the window and GLFW
  ~Application();
  int run();

private:
  void createTriangleResources();
  void destroyTriangleResources();

  GLFWwindow *window_;
  // unsigned int means non negative integer
  // openGL gives many resources int ids. rhese ids are not the resource itself
  // vBuffer and stored vbo id and vArray stores the vao ids
  // shaderProgram_ stores shader program ids
  unsigned int vertexArray_;
  unsigned int vertexBuffer_;
  unsigned int shaderProgram_;

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;
};
} // namespace rubiksim

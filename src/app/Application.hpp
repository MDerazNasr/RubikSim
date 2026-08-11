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
#include "renderer/Mesh.hpp"
#include <glm/glm.hpp>
#include <memory>
#include <vector>

namespace rubiksim {
struct Cubie {
  glm::vec3 position; // stores x, y, z grid position for one small cube
};

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
  void createCubeResources();
  void destroyCubeResources();
  //
  void processInput(float deltaTime);

  GLFWwindow *window_;
  // unsigned int means non negative integer
  // openGL gives many resources int ids. rhese ids are not the resource itself
  // vBuffer and stored vbo id and vArray stores the vao ids
  // shaderProgram_ stores shader program ids
  std::unique_ptr<Mesh> cubeMesh_;
  unsigned int shaderProgram_;
  int modelLoc_{-1};
  int viewLoc_{-1};
  int projectionLoc_{-1};
  int cubiePositionLoc_{-1};

  // camera orbit values
  // yaw rotates left/right around the cubes
  // pitch rotates up/down
  // distance controls zoom
  float cameraYaw_{0.8F};
  float cameraPitch_{0.5F};
  float cameraDistance_{6.0F};

  // used to calc deltaTime
  float lastFrameTime_{0.0F};
  std::vector<Cubie>
      cubies_; // stores all small cubes that make up the rubik's cubeMesh_

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;
};
} // namespace rubiksim
  //

/*
 * Important C++ concept:
 * std::unique_ptr<Mesh> means “this object uniquely owns a Mesh.”
 * Unique means there should be exactly one owner.
 * When the unique_ptr is destroyed, it automatically deletes the Mesh.
 * This is RAII.
 * You no longer need vertexArray_ and vertexBuffer_ in Application because Mesh
 * owns those now.
 */

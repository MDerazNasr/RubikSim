// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <iostream>
#include <stdexcept>
// give this source file access rto glfw functions
#include <GLFW/glfw3.h>

namespace rubiksim {
Application::Application() : window_(nullptr) {
  if (!glfwInit()) {
    throw std::runtime_error("Failed to initialize GLFW");
  }
  // glfwhint tells glfw hwo thr next window should be created
  // context_window verisiob major slect teh majoropengl versioj
  // conext.... minor slect the minor opengl version (3 --> 3.3)
  // context.... core_profile means moden opengl without old legacy
  // GLFW_OPENGL_FORWARD_COMPAT asks for a context compatible with macos
  //
  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  window_ = glfwCreateWindow(1280, 720, "RubikSim", nullptr, nullptr);

  if (window_ == nullptr) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwMakeContextCurrent(window_);
}

Application::~Application() {
  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

int Application::run() {

  std::cout << "RubikSim starting...\n";
  while (!glfwWindowShouldClose(window_)) {
    // sets the color that openGL will use when clearing the screen
    // the numbers are floating point values
    // F means float
    // the 4 vals are red, green, blue, alph (opacitya
    glClearColor(0.08F, 0.10F, 0.12F, 1.0F);
    // tells openGL you want to clear only the color part of the frame buffer
    glClear(GL_COLOR_BUFFER_BIT);

    glfwPollEvents();
    glfwSwapBuffers(window_);
  }
  return 0;
}
} // namespace rubiksim

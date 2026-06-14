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
  window_ = glfwCreateWindow(1280, 720, "RubikSim", nullptr, nullptr);

  if (window_ == nullptr) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }
}

Application::~Application() {
  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

int Application::run() {
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();
  }
  return 0;
}
} // namespace rubiksim

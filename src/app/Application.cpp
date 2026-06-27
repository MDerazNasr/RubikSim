// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <iostream>
#include <stdexcept>
// give this source file access rto glfw functions
#include <GLFW/glfw3.h>
#include <OpenGL/gl3.h>

// anonymous namesce means these helper functions are only visible inside this
// .cpp file
namespace {
unsigned int compileShader(unsigned int type, const char *source) {
  const unsigned int shader = glCreateShader(type);

  glShaderSource(shader, 1, &source, nullptr);
  glCompileShader(shader);

  int success = 0;
  glGetShaderiv(shader, GL_COMPILE_STATUS, &success);

  if (!success) {
    char infoLog[512];
    glGetShaderInfoLog(shader, 512, nullptr, infoLog);
    throw std::runtime_error(infoLog);
  }
  return shader;
}

unsigned int createShaderProgram() {
  const char *vertexShaderSource = "#version 330 core\n"
                                   "layout (location = 0) in vec3 position;\n"
                                   "void main()\n"
                                   "{\n"
                                   " gl_Position = vec4(position, 1.0);\n"
                                   "}\n";
  const char *fragmentShaderSource = "#version 330 core\n"
                                     "out vec4 color;\n"
                                     "void main()\n"
                                     "{\n"
                                     " color = vec4(0.85, 0.20, 0.15, 1.0);\n"
                                     "}\n";

  const unsigned int vertexShader =
      compileShader(GL_VERTEX_SHADER, vertexShaderSource);
  const unsigned int fragmentShader =
      compileShader(GL_FRAGMENT_SHADER, fragmentShaderSource);

  const unsigned int program = glCreateProgram();

  glAttachShader(program, vertexShader);
  glAttachShader(program, fragmentShader);
  glLinkProgram(program);

  int success = 0;
  glGetProgramiv(program, GL_LINK_STATUS, &success);

  if (!success) {
    char infoLog[512];
    glGetProgramInfoLog(program, 512, nullptr, infoLog);
    throw std::runtime_error(infoLog);
  }

  glDeleteShader(vertexShader);
  glDeleteShader(fragmentShader);

  return program;
}
} // namespace

namespace rubiksim {
Application::Application()
    : window_(nullptr), vertexArray_(0), vertexBuffer_(0), shaderProgram_(0) {
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

  window_ = glfwCreateWindow(800, 600, "RubikSim", nullptr, nullptr);

  if (!window_) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwMakeContextCurrent(window_);
  createTriangleResources();
}

void Application::createTriangleResources() {
  // creates an array of floating point numbers that cant be modified -- each
  // group of 3 numbers is one vertex
  const float vertices[] = {0.0F, 0.5F, 0.0F,  -0.5F, -0.5F,
                            0.0F, 0.5F, -0.5F, 0.0F};

  shaderProgram_ = createShaderProgram();

  // asks openGL to create onen VAO
  // means pass the memory address of vertexArray_ so opengl can write the
  // generated id into it
  //
  glGenVertexArrays(1, &vertexArray_);
  // cretaes one VBO
  glGenBuffers(1, &vertexBuffer_);

  // makes this VAO active
  glBindVertexArray(vertexArray_);

  // makes this VBO active as the current vertex data buffer
  glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
  // copies your vertex array into gpu memory
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

  // describes the layout of one vertex
  /*
   * 0 - shader input location 0
   * 3 - three vals per vertex
   * GL Float means each val is a float
   * gl false means dont normalise the vals
   * 3 * size(float) means each egrtex is three floats wide
   * nullptr means the position data stsarst at the start
   */
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), nullptr);
  // enavles shader input location 0
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);
  glBindVertexArray(0);
}
Application::~Application() {
  destroyTriangleResources();

  if (window_) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

void Application::destroyTriangleResources() {
  if (shaderProgram_) {
    glDeleteProgram(shaderProgram_);
  }
  if (vertexBuffer_ != 0) {
    glDeleteBuffers(1, &vertexBuffer_);
  }
  if (vertexArray_ != 0) {
    glDeleteVertexArrays(1, &vertexArray_);
  }
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

    // activates your shader program
    glUseProgram(shaderProgram_);
    // activates your triangle layout
    glBindVertexArray(vertexArray_);
    // tells opengl to draw triangles
    // gl_triangles means every group of tthree vertcies forms one triangle
    glDrawArrays(GL_TRIANGLES, 0, 3);

    glfwPollEvents();
    glfwSwapBuffers(window_);
  }
  return 0;
}
} // namespace rubiksim

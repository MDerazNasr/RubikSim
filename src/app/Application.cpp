// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <iostream>
#include <stdexcept>
// give this source file access rto glfw functions
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

// basic glm vector and matrix types
#include <glm/glm.hpp>
// glm::rotate, glm::translate, glm::perspective
#include <glm/gtc/matrix_transform.hpp>
// glm::value_ptr for sending matrices to openGL
#include <glm/gtc/type_ptr.hpp>

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
  const char *vertexShaderSource =
      // uses glsl version matching opengl 3.3 core
      "#version 330 core\n"
      "layout (location = 0) in vec3 position;\n"
      // the vertex shader nbow accpet sa second vertex attrib
      "layout (location = 1) in vec3 color;\n"
      // uniform means a val sendf from cpp to the shader
      // mat4 means a 4x4 matrix
      // model is the name of the matrix
      // model * vec4 transforms the vertex pis before drawing it
      "uniform mat4 model;\n"
      // the vertex shader sends color to the fragment shader
      "uniform mat4 view;\n" // camera transforms world space to camera space
      // projection * view * model
      // model moves the cube into the world
      // view moves the world relative to the camera
      // projection turns 3D coordinates into screen coordinates
      "uniform mat4 projection;\n" // projecttion transforms 3d camera space to
                                   // screen space
      "out vec3 vertexColor;\n"    // sends color from vertex shader to fragment
      "void main()\n"
      "{\n"
      "vertexColor = color;\n" // pass vertex color
      " gl_Position = projection * view * model * vec4(position, 1.0);\n"
      "}\n";

  const char *fragmentShaderSource =
      "#version 330 core\n"
      // The fragment shader recieves interpolated color
      // Interpolation is a mathematical and computational method of estimating
      // unknown values that fall between known data points.

      "in vec3 vertexColor;\n"
      "out vec4 fragmentColor;\n"
      "void main()\n"
      "{\n"
      // " color = vec4(0.85, 0.20, 0.15, 1.0);\n"
      " fragmentColor = vec4(vertexColor, 1.0);\n"
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
Application::Application() : window_(nullptr), shaderProgram_(0) {
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

  glfwMakeContextCurrent(window_); // makes this windows opengl context active
  // glEnable turns on an opengl capability
  // GL_DEPTH_TEST means opengl should track how far each pixel is from the
  // camera
  // wihtout depth tetsing triangles re drawn mostly in the order you submit
  // them which makes 3d objects look wrong
  glEnable(GL_DEPTH_TEST); // Enables depth testing so nearer triangles hide
                           // farther triangles
  // Create shader and cube mesh after OpenGL context is active
  createCubeResources();
}

void Application::createCubeResources() {
  // create shader and cube mesh after openGL cnext is active
  // creates an array of floating point numbers that cant be modified -- each
  // group of 3 numbers is one vertex
  // const float vertices[] = {0.0F, 0.5F, 0.0F,  -0.5F, -0.5F,
  //                           0.0F, 0.5F, -0.5F, 0.0F};

  // the followign is a dynamic array of vertex structs
  const std::vector<Vertex> vertices = {
      {-0.5F, -0.5F, -0.5F, 1.0F, 0.0F, 0.0F}, // 0 back bottom left, red.
      {0.5F, -0.5F, -0.5F, 0.0F, 1.0F, 0.0F},  // 1 back bottom right, green.
      {0.5F, 0.5F, -0.5F, 0.0F, 0.0F, 1.0F},   // 2 back top right, blue.
      {-0.5F, 0.5F, -0.5F, 1.0F, 1.0F, 0.0F},  // 3 back top left, yellow.
      {-0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 1.0F},  // 4 front bottom left,magenta.
      {0.5F, -0.5F, 0.5F, 0.0F, 1.0F, 1.0F},   // 5 front bottom right, cyan.
      {0.5F, 0.5F, 0.5F, 1.0F, 1.0F, 1.0F},    // 6 front top right, white.
      {-0.5F, 0.5F, 0.5F, 1.0F, 0.5F, 0.0F}    // 7 front top left, orange.
  };

  // every vertex has 6 floats
  // x y z r g b
  // now indices -
  const std::vector<unsigned int> indices = {
      4, 5, 6, 4, 6, 7, // Front face.
      1, 0, 3, 1, 3, 2, // Back face.
      0, 4, 7, 0, 7, 3, // Left face.
      5, 1, 2, 5, 2, 6, // Right face.
      3, 7, 6, 3, 6, 2, // Top face.
      0, 1, 5, 0, 5, 4  // Bottom face.
  };

  shaderProgram_ = createShaderProgram();
  cubeMesh_ = std::make_unique<Mesh>(vertices, indices);
  cubies_.clear(); //removes any exisiting cubies from the vecrtor 
    
  for (int x = -1; x <= 1; ++x) { // loop through left, center, right
      for (int y = -1; y <= 1; ++y) { // loop through back center front
          for (int z = -1; z <= 1; ++z){ //loop through back center Front
              // push_back appendsa a new item to a std::vector
              cubies_.push_back(Cubie{
                      glm::vec3(
                              // explicity coverts int to float
                              static_cast<float>(x),
                              static_cast<float>(y),
                              static_cast<float>(z)
                              )
                      });

          }
      }
  }
}
Application::~Application() {
  destroyCubeResources();

  if (window_) {
    glfwDestroyWindow(window_);
  }
  glfwTerminate();
}

void Application::destroyCubeResources() {
  // reset destroys the objects insdie the unique_ptr
  // this calls Mesh::~Mesh()
  // that destructor deletes the VAO, VBO, & EBO
  // Then Application del only the shader program
  cubeMesh_.reset(); // Destroys the Mesh, which delets VAO, VBO and EBO
  if (shaderProgram_) {
    glDeleteProgram(shaderProgram_); // deletes the GPU shader program
    shaderProgram_ = 0;              // Marks the handle as empty
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
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    // activates your shader program
    glUseProgram(shaderProgram_);
    // activates your triangle layout

    // glfwGetTime returns seconds as double. static_cast<float> converts it to
    // float
    // staic... is an explicit type conversion, it says ik this val is a double
    // but i want a float
    const float time = static_cast<float>(glfwGetTime());

    // create a rotation matric around the z axis
    //  const glm::mat4 model creates a matrix var that cant be reassigned
    //  glm::mat4(1.0 F) creates an identity matrix which is when it changes
    //  nothing by itself glm..vec3 ... create a 3d vecrtore pointing along the
    //  Z axis
    const glm::mat4 baseRotation = glm::rotate(
        glm::mat4(1.0F), time,
        glm::vec3(0.5F, 1.0F, 0.0F)); // rotate cube around diagonal axis
    const glm::mat4 view = glm::translate(
        glm::mat4(1.0F),
        glm::vec3(0.0F, 0.0F, -3.0F)); // Move away from the camera.
    const glm::mat4 projection =
        glm::perspective(glm::radians(45.0F), 800.0F / 600.0F, 0.1F,
                         100.0F); // Perspective camera lens.

    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "view"), 1,
                       GL_FALSE, glm::value_ptr(view));
    glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "projection"), 1,
                       GL_FALSE, glm::value_ptr(projection));

    for (const Cubie &cubie : cubies_) {
      const glm::mat4 model =
          baseRotation *
          glm::translate(glm::mat4(1.0F), cubie.position * 1.1F) *
          glm::scale(glm::mat4(1.0F), glm::vec3(0.3F));

      glUniformMatrix4fv(glGetUniformLocation(shaderProgram_, "model"), 1,
                         GL_FALSE, glm::value_ptr(model));
      cubeMesh_->draw();
    }

    glfwPollEvents();
    glfwSwapBuffers(window_);
  }
  return 0;
}
} // namespace rubiksim

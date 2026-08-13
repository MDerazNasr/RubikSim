// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <ios>
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
#include <cmath>
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
      "layout (location = 1) in vec3 normal;\n"
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
      "out vec3 vertexNormal;\n" // sends normal from vertex shader to fragment
      "void main()\n"
      "{\n"
      "vertexNormal = normal;\n" // pass vertex normal
      " gl_Position = projection * view * model * vec4(position, 1.0);\n"
      "}\n";

  const char *fragmentShaderSource =
      "#version 330 core\n"
      "in vec3 vertexNormal;\n"
      "uniform vec3 frontColor;\n"
      "uniform vec3 backColor;\n"
      "uniform vec3 leftColor;\n"
      "uniform vec3 rightColor;\n"
      "uniform vec3 topColor;\n"
      "uniform vec3 bottomColor;\n"
      "out vec4 fragmentColor;\n"
      "void main()\n"
      "{\n"
      " vec3 color = vec3(0.05, 0.05, 0.05);\n"
      "\n"
      " if (vertexNormal.z > 0.5) {\n"
      "   color = frontColor;\n"
      " } else if (vertexNormal.z < -0.5) {\n"
      "   color = backColor;\n"
      " } else if (vertexNormal.x < -0.5) {\n"
      "   color = leftColor;\n"
      " } else if (vertexNormal.x > 0.5) {\n"
      "   color = rightColor;\n"
      " } else if (vertexNormal.y > 0.5) {\n"
      "   color = topColor;\n"
      " } else if (vertexNormal.y < -0.5) {\n"
      "   color = bottomColor;\n"
      " }\n"
      "\n"
      " vec3 lightDir = normalize(vec3(0.5, 1.0, 0.8));\n"
      " float diff = max(dot(vertexNormal, lightDir), 0.0);\n"
      " vec3 shadedColor = color * (0.45 + 0.55 * diff);\n"
      " fragmentColor = vec4(shadedColor, 1.0);\n"
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

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
  glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GLFW_TRUE);

  window_ = glfwCreateWindow(800, 600, "RubikSim", nullptr, nullptr);

  if (!window_) {
    glfwTerminate();
    throw std::runtime_error("Failed to create GLFW window");
  }

  glfwMakeContextCurrent(window_); // makes this window's opengl context active
  glfwSwapInterval(1);             // Enable vsync for smooth frame swapping
  glfwShowWindow(window_);         // Ensure window is visible on macOS
  glfwFocusWindow(window_);        // Bring window to front

  glEnable(GL_DEPTH_TEST); // Enables depth testing
  createCubeResources();
}

void Application::createCubeResources() {
  // This cube uses 24 vertices (4 per face) so each face has its own color.
  const std::vector<Vertex> vertices = {
      // Front face, z = +0.5, normal (0, 0, 1)
      {-0.5F, -0.5F, 0.5F, 0.0F, 0.0F, 1.0F},
      {0.5F, -0.5F, 0.5F, 0.0F, 0.0F, 1.0F},
      {0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 1.0F},
      {-0.5F, 0.5F, 0.5F, 0.0F, 0.0F, 1.0F},

      // Back face, z = -0.5, normal (0, 0, -1)
      {0.5F, -0.5F, -0.5F, 0.0F, 0.0F, -1.0F},
      {-0.5F, -0.5F, -0.5F, 0.0F, 0.0F, -1.0F},
      {-0.5F, 0.5F, -0.5F, 0.0F, 0.0F, -1.0F},
      {0.5F, 0.5F, -0.5F, 0.0F, 0.0F, -1.0F},

      // Left face, x = -0.5, normal (-1, 0, 0)
      {-0.5F, -0.5F, -0.5F, -1.0F, 0.0F, 0.0F},
      {-0.5F, -0.5F, 0.5F, -1.0F, 0.0F, 0.0F},
      {-0.5F, 0.5F, 0.5F, -1.0F, 0.0F, 0.0F},
      {-0.5F, 0.5F, -0.5F, -1.0F, 0.0F, 0.0F},

      // Right face, x = +0.5, normal (1, 0, 0)
      {0.5F, -0.5F, 0.5F, 1.0F, 0.0F, 0.0F},
      {0.5F, -0.5F, -0.5F, 1.0F, 0.0F, 0.0F},
      {0.5F, 0.5F, -0.5F, 1.0F, 0.0F, 0.0F},
      {0.5F, 0.5F, 0.5F, 1.0F, 0.0F, 0.0F},

      // Top face, y = +0.5, normal (0, 1, 0)
      {-0.5F, 0.5F, 0.5F, 0.0F, 1.0F, 0.0F},
      {0.5F, 0.5F, 0.5F, 0.0F, 1.0F, 0.0F},
      {0.5F, 0.5F, -0.5F, 0.0F, 1.0F, 0.0F},
      {-0.5F, 0.5F, -0.5F, 0.0F, 1.0F, 0.0F},

      // Bottom face, y = -0.5, normal (0, -1, 0)
      {-0.5F, -0.5F, -0.5F, 0.0F, -1.0F, 0.0F},
      {0.5F, -0.5F, -0.5F, 0.0F, -1.0F, 0.0F},
      {0.5F, -0.5F, 0.5F, 0.0F, -1.0F, 0.0F},
      {-0.5F, -0.5F, 0.5F, 0.0F, -1.0F, 0.0F},
  };

  const std::vector<unsigned int> indices = {
      0,  1,  2,  0,  2,  3,  // Front
      4,  5,  6,  4,  6,  7,  // Back
      8,  9,  10, 8,  10, 11, // Left
      12, 13, 14, 12, 14, 15, // Right
      16, 17, 18, 16, 18, 19, // Top
      20, 21, 22, 20, 22, 23  // Bottom
  };

  shaderProgram_ = createShaderProgram();
  modelLoc_ = glGetUniformLocation(shaderProgram_, "model");
  viewLoc_ = glGetUniformLocation(shaderProgram_, "view");
  projectionLoc_ = glGetUniformLocation(shaderProgram_, "projection");
  frontColorLoc_ = glGetUniformLocation(shaderProgram_, "frontColor");
  backColorLoc_ = glGetUniformLocation(shaderProgram_, "backColor");
  leftColorLoc_ = glGetUniformLocation(shaderProgram_, "leftColor");
  rightColorLoc_ = glGetUniformLocation(shaderProgram_, "rightColor");
  topColorLoc_ = glGetUniformLocation(shaderProgram_, "topColor");
  bottomColorLoc_ = glGetUniformLocation(shaderProgram_, "bottomColor");

  cubeMesh_ = std::make_unique<Mesh>(vertices, indices);
  cubies_.clear();

  for (int x = -1; x <= 1; ++x) {
    for (int y = -1; y <= 1; ++y) {
      for (int z = -1; z <= 1; ++z) {
        const glm::vec3 black(0.05F, 0.05F, 0.05F);
        const glm::vec3 green(0.0F, 0.8F, 0.1F);
        const glm::vec3 blue(0.0F, 0.2F, 1.0F);
        const glm::vec3 orange(1.0F, 0.45F, 0.0F);
        const glm::vec3 red(0.9F, 0.0F, 0.0F);
        const glm::vec3 white(1.0F, 1.0F, 1.0F);
        const glm::vec3 yellow(1.0F, 0.9F, 0.0F);

        cubies_.push_back(Cubie{
            glm::vec3(static_cast<float>(x), static_cast<float>(y),
                      static_cast<float>(z)),
            z == 1 ? green : black,
            z == -1 ? blue : black,
            x == -1 ? orange : black,
            x == 1 ? red : black,
            y == 1 ? white : black,
            y == -1 ? yellow : black,
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
  cubeMesh_.reset();
  if (shaderProgram_) {
    glDeleteProgram(shaderProgram_);
    shaderProgram_ = 0;
  }
  modelLoc_ = -1;
  viewLoc_ = -1;
  projectionLoc_ = -1;

  frontColorLoc_ = -1;
  backColorLoc_ = -1;
  leftColorLoc_ = -1;
  rightColorLoc_ = -1;
  topColorLoc_ = -1;
  bottomColorLoc_ = -1;
}

void Application::processInput(float deltaTime) {
  if (glfwGetKey(window_, GLFW_KEY_R) == GLFW_PRESS) {
    selectedFace_ = SelectedFace::Right;
  }
  if (glfwGetKey(window_, GLFW_KEY_L) == GLFW_PRESS) {
    selectedFace_ = SelectedFace::Left;
  }
  if (glfwGetKey(window_, GLFW_KEY_U) == GLFW_PRESS) {
    selectedFace_ = SelectedFace::Top;
  }
  if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS) {
    selectedFace_ = SelectedFace::Bottom;
  }
  if (glfwGetKey(window_, GLFW_KEY_F) == GLFW_PRESS) {
    selectedFace_ = SelectedFace::Front;
  }
  if (glfwGetKey(window_, GLFW_KEY_B) == GLFW_PRESS) {
    selectedFace_ = SelectedFace::Back;
  }

  // if escape is pressed, tell GLFW the window should close
  if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }

  // These numbers control how fast the camera moves
  // The values are multiplied by deltaTime so speed is frame-rate independant
  const float orbitSpeed_ = 1.5F;
  const float zoomSpeed = 3.0F;

  if (glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS) {
    cameraYaw_ -= orbitSpeed_ * deltaTime;
  }
  if (glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS) {
    cameraYaw_ += orbitSpeed_ * deltaTime;
  }
  if (glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
    cameraPitch_ += orbitSpeed_ * deltaTime;
  }
  if (glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
    cameraPitch_ -= orbitSpeed_ * deltaTime;
  }
  if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
    cameraDistance_ -= zoomSpeed * deltaTime;
  }
  if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
    cameraDistance_ += zoomSpeed * deltaTime;
  }

  // Space starts a face turn.
  //
  // Important:
  // We only start a new turn if:
  // 1. Space is pressed now.
  // 2. Space was not already pressed last frame.
  // 3. No turn is currently happening.
  //
  // This means one key press creates one clean animation.
  const bool spaceIsPressed = glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS;
  if (spaceIsPressed && !spaceWasPressed_ && !isTurning_) {
    // The animation is now active.
    isTurning_ = true;

    // Freeze which face is turning.
    //
    // selectedFace_ can still change later if the user presses R/L/U/D/F/B,
    // but turningFace_ keeps this animation tied to the original face.
    turningFace_ = selectedFace_;

    // Start from 0 radians.
    // The animation update below will increase this over time.
    turnAngle_ = 0.0F;
  }
  spaceWasPressed_ = spaceIsPressed;

  // If a face turn is active, advance the angle a little this frame.
  if (isTurning_) {
    // A Rubik's cube face turn is 90 degrees.
    //
    // GLM rotation uses radians, so we convert degrees to radians.
    const float targetAngle = glm::radians(90.0F);

    // This is the turn speed.
    //
    // 180 degrees per second means a 90 degree turn takes about half a second.
    const float turnSpeed = glm::radians(180.0F);

    // Increase the current angle by:
    // speed * time passed since last frame.
    //
    // This makes the animation frame-rate independent.
    turnAngle_ += turnSpeed * deltaTime;

    // If we reached or passed 90 degrees, clamp exactly to 90.
    // Without this, the angle might stop at 91.2 or 90.6 depending on frame
    // time.
    if (turnAngle_ >= targetAngle) {
      turnAngle_ = targetAngle;

      // Stop the animation.
      //
      // Important limitation:
      // At this point the cubies are only visually rotated.
      // In the next section, this is where we will permanently update cubie
      // positions and sticker orientations.
      isTurning_ = false;
    }
  }

  // clamp means keep a value inside a safe range
  // we stop pitch before it goes fully vertical because that can make
  // camera movement confusing
  //
  if (cameraPitch_ > 1.2F) {
    cameraPitch_ = 1.2F;
  }
  if (cameraPitch_ < -1.2F) {
    cameraPitch_ = -1.2F;
  }

  // Prevent zooming inside the cube or too far away
  if (cameraDistance_ < 3.0F) {
    cameraDistance_ = 3.0F;
  }
  if (cameraDistance_ > 12.0F) {
    cameraDistance_ = 12.0F;
  }
}

bool Application::isCubieInFace(const Cubie &cubie, SelectedFace face) const {
  // a cubie pos is one of -1, 0, or 1 on each axis
  // x == 1 means right layer
  // x == -1 means left layer
  // y == 1 means top layer
  // y == -1 means bottom layer
  // z == 1 means front layer
  // z == -1 means back layer
  if (face == SelectedFace::Right) {
    return cubie.position.x == 1.0F;
  }

  if (face == SelectedFace::Left) {
    return cubie.position.x == -1.0F;
  }
  if (face == SelectedFace::Top) {
    return cubie.position.y == 1.0F;
  }

  if (face == SelectedFace::Bottom) {
    return cubie.position.y == -1.0F;
  }
  if (face == SelectedFace::Front) {
    return cubie.position.z == 1.0F;
  }

  if (face == SelectedFace::Back) {
    return cubie.position.z == -1.0F;
  }

  return false;
}

glm::vec3 rotationAxisForFace(SelectedFace face) {
  // A rotation axis is the imaginary line that an object spins around.
  //
  // If you rotate the Right or Left face, the cubies spin around the X axis.
  // If you rotate the Top or Bottom face, the cubies spin around the Y axis.
  // If you rotate the Front or Back face, the cubies spin around the Z axis.
  if (face == SelectedFace::Right || face == SelectedFace::Left) {
    return glm::vec3(1.0F, 0.0F, 0.0F);
  }
  if (face == SelectedFace::Top || face == SelectedFace::Bottom) {
    return glm::vec3(0.0F, 1.0F, 0.0F);
  }

  return glm::vec3(0.0F, 0.0F, 1.0F);
}
int Application::run() {
  std::cout << "RubikSim starting...\n";
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    int displayW = 0, displayH = 0;
    glfwGetFramebufferSize(window_, &displayW, &displayH);
    if (displayW > 0 && displayH > 0) {
      glViewport(0, 0, displayW, displayH);
    }

    glClearColor(0.08F, 0.10F, 0.12F, 1.0F);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(shaderProgram_);

    // returns total seconda sicne GLFW starteda
    const float currentTime = static_cast<float>(glfwGetTime());

    // deltatime is the time between this frsame and the previous frame
    const float deltaTime = currentTime - lastFrameTime_;
    lastFrameTime_ = currentTime;

    processInput(deltaTime);

    // the cuvbe utself is no longer rotating automatically
    // we keep the model group stull and move the camera instead
    const glm::mat4 baseRotation = glm::mat4(1.0F);

    // convert yaw, pitch, and diatance into a 3D camera position
    // cos(pitch) controsl the horizontal disatance
    // sin(pitch) controls the height
    // sin(yaw ) and cos(yaw) move the camera around the cube
    const glm::vec3 cameraPosition(
        cameraDistance_ * std::cos(cameraPitch_) * std::sin(cameraYaw_),
        cameraDistance_ * std::sin(cameraPitch_),
        cameraDistance_ * std::cos(cameraPitch_) * std::cos(cameraYaw_));

    // lookAt creates a camera view matrix
    // first argument: whre the camera is
    // second argument: what the camera looks at
    // third argument: which direction counts as up
    const glm::mat4 view =
        glm::lookAt(cameraPosition, glm::vec3(0.0F, 0.0F, 0.0F),
                    glm::vec3(0.0F, 1.0F, 0.0F));

    float aspect =
        (displayH > 0)
            ? (static_cast<float>(displayW) / static_cast<float>(displayH))
            : (800.0F / 600.0F);
    const glm::mat4 projection =
        glm::perspective(glm::radians(45.0F), aspect, 0.1F, 100.0F);

    if (viewLoc_ != -1) {
      glUniformMatrix4fv(viewLoc_, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (projectionLoc_ != -1) {
      glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE,
                         glm::value_ptr(projection));
    }

    const float cubieSpacing = 1.02F;
    const float cubieScale = 0.48F;

    for (const Cubie &cubie : cubies_) {
      // Decide whether this specific cubie should receive the layer rotation.
      //
      // A cubie should rotate if:
      // 1. a turn angle is currently visible
      // 2. it belongs to the face that started the turn
      //
      // We use turningFace_, not selectedFace_, because selectedFace_ can
      // change while the animation is running.
      const bool cubieIsTurning =
          turnAngle_ > 0.0F && isCubieInFace(cubie, turningFace_);

      // layerRotation is either:
      // - a real rotation matrix for cubies in the turning layer
      // - identity matrix for cubies outside the turning layer
      //
      // Identity matrix means "do nothing."
      const glm::mat4 layerRotation =
          cubieIsTurning ? glm::rotate(glm::mat4(1.0F), turnAngle_,
                                       rotationAxisForFace(turningFace_))
                         : glm::mat4(1.0F);

      // Matrix order matters.
      //
      // Written order:
      // baseRotation * layerRotation * translate * scale
      //
      // Applied order to the vertex:
      // 1. scale the cubie to a small cube
      // 2. move the cubie to its grid position
      // 3. rotate the whole selected layer if this cubie is part of it
      // 4. apply any whole-cube base rotation
      //
      // For now, baseRotation is identity, so it does nothing.
      const glm::mat4 model =
          baseRotation * layerRotation *
          glm::translate(glm::mat4(1.0F), cubie.position * cubieSpacing) *
          glm::scale(glm::mat4(1.0F), glm::vec3(cubieScale));

      if (modelLoc_ != -1) {
        glUniformMatrix4fv(modelLoc_, 1, GL_FALSE, glm::value_ptr(model));
      }

      glUniform3fv(frontColorLoc_, 1, glm::value_ptr(cubie.frontColor));
      glUniform3fv(backColorLoc_, 1, glm::value_ptr(cubie.backColor));
      glUniform3fv(leftColorLoc_, 1, glm::value_ptr(cubie.leftColor));
      glUniform3fv(rightColorLoc_, 1, glm::value_ptr(cubie.rightColor));
      glUniform3fv(topColorLoc_, 1, glm::value_ptr(cubie.topColor));
      glUniform3fv(bottomColorLoc_, 1, glm::value_ptr(cubie.bottomColor));

      cubeMesh_->draw();
    }

    glfwSwapBuffers(window_);
  }
  return 0;
}
} // namespace rubiksim

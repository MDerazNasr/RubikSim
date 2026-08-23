// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <ios>
#include <iostream>
#include <limits>
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
      "uniform int useOverrideColor;\n"
      "uniform vec3 overrideColor;\n"
      "uniform vec3 cubiePosition;\n"
      "uniform int highlightEnabled;\n"
      "uniform vec3 highlightedCubiePosition;\n"
      "uniform vec3 highlightedFaceNormal;\n"
      "out vec4 fragmentColor;\n"
      "void main()\n"
      "{\n"
      " if (useOverrideColor == 1) {\n"
      "   fragmentColor = vec4(overrideColor, 1.0);\n"
      "   return;\n"
      " }\n"
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
      "\n"
      " if (highlightEnabled == 1 &&\n"
      "     distance(cubiePosition, highlightedCubiePosition) < 0.01 &&\n"
      "     dot(vertexNormal, highlightedFaceNormal) > 0.5) {\n"
      "   shadedColor = mix(shadedColor, vec3(1.0, 1.0, 0.2), 0.65);\n"
      " }\n"
      " fragmentColor = vec4(shadedColor, 1.0);\n"
      "}\n";
      /*
       *   What this means:

        distance(cubiePosition, highlightedCubiePosition) < 0.01

        checks that we are drawing the hovered cubie.

        dot(vertexNormal, highlightedFaceNormal) > 0.5

        checks that we are drawing the exact hovered face, not the whole
        cubie.
      */
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

  useOverrideColorLoc_ =
      glGetUniformLocation(shaderProgram_, "useOverrideColor");
  overrideColorLoc_ = glGetUniformLocation(shaderProgram_, "overrideColor");

  cubiePositionLoc_ = glGetUniformLocation(shaderProgram_, "cubiePosition");
  highlightEnabledLoc_ =
      glGetUniformLocation(shaderProgram_, "highlightEnabled");
  highlightedCubiePositionLoc_ =
      glGetUniformLocation(shaderProgram_, "highlightedCubiePosition");
  highlightedFaceNormalLoc_ =
      glGetUniformLocation(shaderProgram_, "highlightedFaceNormal");

  cubeMesh_ = std::make_unique<Mesh>(vertices, indices);
  resetCubeState();
}

void Application::resetCubeState() {
  // Cancel any turn animation that might currently be in progress.
  //
  // Reset means "go directly back to solved."
  // We do not want to finish the current animation first.
  isTurning_ = false;
  turnAngle_ = 0.0F;

  // Also cancel mouse drag state.
  //
  // Without this, the user could hold the mouse down, reset, and then release
  // into old drag state from before the reset.
  isDraggingCamera_ = false;
  isDraggingCube_ = false;
  hasStartedMouseTurn_ = false;
  activePick_ = MousePick{};
  hoverPick_ = MousePick{};

  // Rebuild all cubies from scratch in solved positions.
  //
  // This is simpler and safer than trying to reverse all previous moves.
  // The solved cube is just the 27 grid positions from -1 to +1 on x/y/z.
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

  useOverrideColorLoc_ = -1;
  overrideColorLoc_ = -1;

  cubiePositionLoc_ = -1;
  highlightEnabledLoc_ = -1;
  highlightedCubiePositionLoc_ = -1;
  highlightedFaceNormalLoc_ = -1;
}

void Application::processInput(float deltaTime) {
  // if escape is pressed, tell GLFW the window should close
  if (glfwGetKey(window_, GLFW_KEY_ESCAPE) == GLFW_PRESS) {
    glfwSetWindowShouldClose(window_, GLFW_TRUE);
  }

  // Backspace or Enter resets the cube to solved state.
  //
  // We check for "pressed now" and "was not pressed last frame" so one key
  // press creates one reset.
  const bool resetIsPressed =
      glfwGetKey(window_, GLFW_KEY_BACKSPACE) == GLFW_PRESS ||
      glfwGetKey(window_, GLFW_KEY_ENTER) == GLFW_PRESS;

  if (resetIsPressed && !resetWasPressed_) {
    resetCubeState();
  }
  resetWasPressed_ = resetIsPressed;

  // W and S still control zoom for now.
  // Later we can move zoom to the mouse wheel.
  const float zoomSpeed = 3.0F;

  if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS) {
    cameraDistance_ -= zoomSpeed * deltaTime;
  }

  if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS) {
    cameraDistance_ += zoomSpeed * deltaTime;
  }

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

      // Commit the finished move to the actual cube data.
      //
      // Until this call, the turn only exists as a temporary drawing matrix.
      // After this call, cubie positions and sticker colors store the turn.
      applyTurnToCubeState();

      // Reset the temporary visual animation.
      //
      // Why reset to 0?
      // Because the cubies' stored positions now contain the finished turn.
      // If we kept turnAngle_ at 90 degrees, we would draw the same turn twice:
      // once from state, once from animation.
      turnAngle_ = 0.0F;

      // Stop the animation.
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

glm::vec3 rotationAxisForTurnAxis(TurnAxis axis) {
  if (axis == TurnAxis::X) {
    return glm::vec3(1.0F, 0.0F, 0.0F);
  }

  if (axis == TurnAxis::Y) {
    return glm::vec3(0.0F, 1.0F, 0.0F);
  }

  return glm::vec3(0.0F, 0.0F, 1.0F);
}

bool Application::isCubieInTurningLayer(const Cubie &cubie) const {
  // turningAxis_ chooses which coordinate we inspect.
  // turningLayer_ chooses which slice moves.
  //
  // Example:
  // axis X, layer 1 means every cubie where x == 1.
  // axis Y, layer 0 means every cubie where y == 0.
  // axis Z, layer -1 means every cubie where z == -1.
  if (turningAxis_ == TurnAxis::X) {
    return static_cast<int>(cubie.position.x) == turningLayer_;
  }

  if (turningAxis_ == TurnAxis::Y) {
    return static_cast<int>(cubie.position.y) == turningLayer_;
  }

  return static_cast<int>(cubie.position.z) == turningLayer_;
}

void Application::startTurn(const Move &move) {
  // Do not start a second turn while one is already animating.
  //
  // This guard matters because later scramble will try to run many moves.
  // For now, we only allow one active move at a time.
  if (isTurning_) {
    return;
  }

  // Copy the move data into the animation state.
  //
  // The draw loop and applyTurnToCubeState() already know how to use these
  // three values.
  turningAxis_ = move.axis;
  turningLayer_ = move.layer;
  turnDirection_ = move.direction;

  // Start the visual animation from 0 degrees.
  isTurning_ = true;
  turnAngle_ = 0.0F;
}

void Application::applyTurnToCubeState() {
  // A finished turn has three parts:
  //
  // 1. Which axis the layer rotated around: X, Y, or Z.
  // 2. Which layer moved on that axis: -1, 0, or 1.
  // 3. Which direction it rotated: +1 or -1.
  //
  // The draw loop used those values to rotate the layer visually.
  // This function uses the same values to update the real cube data.
  const bool positiveTurn = turnDirection_ > 0.0F;

  for (Cubie &cubie : cubies_) {
    if (!isCubieInTurningLayer(cubie)) {
      continue;
    }

    const float oldX = cubie.position.x;
    const float oldY = cubie.position.y;
    const float oldZ = cubie.position.z;

    const glm::vec3 oldFront = cubie.frontColor;
    const glm::vec3 oldBack = cubie.backColor;
    const glm::vec3 oldLeft = cubie.leftColor;
    const glm::vec3 oldRight = cubie.rightColor;
    const glm::vec3 oldTop = cubie.topColor;
    const glm::vec3 oldBottom = cubie.bottomColor;

    if (turningAxis_ == TurnAxis::X) {
      // Rotate position around X.
      //
      // X stays fixed because this layer spins around the X axis.
      if (positiveTurn) {
        cubie.position.y = -oldZ;
        cubie.position.z = oldY;

        // Sticker colors rotate with the cubie.
        //
        // A +90 X turn moves:
        // top -> front
        // front -> bottom
        // bottom -> back
        // back -> top
        cubie.topColor = oldBack;
        cubie.frontColor = oldTop;
        cubie.bottomColor = oldFront;
        cubie.backColor = oldBottom;
      } else {
        cubie.position.y = oldZ;
        cubie.position.z = -oldY;

        // A -90 X turn is the opposite cycle.
        cubie.topColor = oldFront;
        cubie.backColor = oldTop;
        cubie.bottomColor = oldBack;
        cubie.frontColor = oldBottom;
      }

      cubie.leftColor = oldLeft;
      cubie.rightColor = oldRight;
    } else if (turningAxis_ == TurnAxis::Y) {
      // Rotate position around Y.
      //
      // Y stays fixed because this layer spins around the Y axis.
      if (positiveTurn) {
        cubie.position.x = oldZ;
        cubie.position.z = -oldX;

        // A +90 Y turn moves:
        // front -> right
        // right -> back
        // back -> left
        // left -> front
        cubie.frontColor = oldLeft;
        cubie.rightColor = oldFront;
        cubie.backColor = oldRight;
        cubie.leftColor = oldBack;
      } else {
        cubie.position.x = -oldZ;
        cubie.position.z = oldX;

        // A -90 Y turn is the opposite cycle.
        cubie.frontColor = oldRight;
        cubie.leftColor = oldFront;
        cubie.backColor = oldLeft;
        cubie.rightColor = oldBack;
      }

      cubie.topColor = oldTop;
      cubie.bottomColor = oldBottom;
    } else {
      // Rotate position around Z.
      //
      // Z stays fixed because this layer spins around the Z axis.
      if (positiveTurn) {
        cubie.position.x = -oldY;
        cubie.position.y = oldX;

        // A +90 Z turn moves:
        // right -> top
        // top -> left
        // left -> bottom
        // bottom -> right
        cubie.rightColor = oldBottom;
        cubie.topColor = oldRight;
        cubie.leftColor = oldTop;
        cubie.bottomColor = oldLeft;
      } else {
        cubie.position.x = oldY;
        cubie.position.y = -oldX;

        // A -90 Z turn is the opposite cycle.
        cubie.rightColor = oldTop;
        cubie.bottomColor = oldRight;
        cubie.leftColor = oldBottom;
        cubie.topColor = oldLeft;
      }

      cubie.frontColor = oldFront;
      cubie.backColor = oldBack;
    }
  }
}

MousePick Application::pickCubie(double mouseX, double mouseY,
                                 const glm::mat4 &view,
                                 const glm::mat4 &projection, int displayW,
                                 int displayH) const {
  MousePick bestPick;

  if (displayW <= 0 || displayH <= 0) {
    return bestPick;
  }

  // Convert a 2D mouse position into a 3D ray.
  //
  // GLFW mouse y starts at the top of the window.
  // OpenGL viewport y starts at the bottom.
  // That is why we use displayH - mouseY.
  //
  // Important macOS/Retina detail:
  // glfwGetCursorPos gives mouse coordinates in window coordinates.
  // glViewport uses framebuffer coordinates.
  //
  // On a normal display those are often the same size.
  // On a Retina display the framebuffer can be 2x larger than the window.
  //
  // So before unProject, we scale the mouse position from window space into
  // framebuffer space. Without this, the picking ray can be offset and miss the
  // cube, which makes hover highlighting look like it is not working.
  int windowW = 0;
  int windowH = 0;
  glfwGetWindowSize(window_, &windowW, &windowH);
  if (windowW <= 0 || windowH <= 0) {
    return bestPick;
  }

  const float framebufferMouseX =
      static_cast<float>(mouseX) * static_cast<float>(displayW) /
      static_cast<float>(windowW);
  const float framebufferMouseY =
      static_cast<float>(mouseY) * static_cast<float>(displayH) /
      static_cast<float>(windowH);

  const glm::vec4 viewport(0.0F, 0.0F, static_cast<float>(displayW),
                           static_cast<float>(displayH));
  const glm::vec3 nearPoint =
      glm::unProject(glm::vec3(framebufferMouseX,
                               static_cast<float>(displayH) -
                                   framebufferMouseY,
                               0.0F),
                     view, projection, viewport);
  const glm::vec3 farPoint =
      glm::unProject(glm::vec3(framebufferMouseX,
                               static_cast<float>(displayH) -
                                   framebufferMouseY,
                               1.0F),
                     view, projection, viewport);

  const glm::vec3 rayOrigin = nearPoint;
  const glm::vec3 rayDirection = glm::normalize(farPoint - nearPoint);

  const float cubieSpacing = 0.50F;
  const float cubieScale = 0.46F;
  const float halfSize = cubieScale * 0.5F;
  float bestDistance = std::numeric_limits<float>::max();

  for (const Cubie &cubie : cubies_) {
    const glm::vec3 center = cubie.position * cubieSpacing;
    const glm::vec3 boxMin = center - glm::vec3(halfSize);
    const glm::vec3 boxMax = center + glm::vec3(halfSize);

    float tMin = 0.0F;
    float tMax = bestDistance;
    bool hit = true;

    for (int axis = 0; axis < 3; ++axis) {
      const float origin = rayOrigin[axis];
      const float direction = rayDirection[axis];

      if (std::abs(direction) < 0.0001F) {
        if (origin < boxMin[axis] || origin > boxMax[axis]) {
          hit = false;
          break;
        }
        continue;
      }

      float t1 = (boxMin[axis] - origin) / direction;
      float t2 = (boxMax[axis] - origin) / direction;

      if (t1 > t2) {
        const float temp = t1;
        t1 = t2;
        t2 = temp;
      }

      if (t1 > tMin) {
        tMin = t1;
      }

      if (t2 < tMax) {
        tMax = t2;
      }

      if (tMin > tMax) {
        hit = false;
        break;
      }
    }

    if (!hit || tMin < 0.0F || tMin >= bestDistance) {
      continue;
    }

    const glm::vec3 hitPoint = rayOrigin + rayDirection * tMin;
    const glm::vec3 localHit = hitPoint - center;
    glm::vec3 faceNormal(0.0F);

    const float absX = std::abs(localHit.x);
    const float absY = std::abs(localHit.y);
    const float absZ = std::abs(localHit.z);

    if (absX >= absY && absX >= absZ) {
      faceNormal.x = localHit.x >= 0.0F ? 1.0F : -1.0F;
    } else if (absY >= absX && absY >= absZ) {
      faceNormal.y = localHit.y >= 0.0F ? 1.0F : -1.0F;
    } else {
      faceNormal.z = localHit.z >= 0.0F ? 1.0F : -1.0F;
    }

    bestDistance = tMin;
    bestPick.hit = true;
    bestPick.cubiePosition = cubie.position;
    bestPick.faceNormal = faceNormal;
    bestPick.distance = tMin;
  }

  return bestPick;
}

TurnAxis strongAxisFromVector(const glm::vec3 &direction) {
  // this function answers - which world axis is this direction closest to
  // Example:
  // direction is mostly left/right in world space -> X
  // direction is mostly up/down in world space    -> Y
  // direction is mostly front/back in world space -> Z
  const glm::vec3 absoluteDirection(
      std::abs(direction.x), std::abs(direction.y), std::abs(direction.z));

  if (absoluteDirection.x >= absoluteDirection.y &&
      absoluteDirection.x >= absoluteDirection.z) {
    return TurnAxis::X;
  }

  if (absoluteDirection.y >= absoluteDirection.x &&
      absoluteDirection.y >= absoluteDirection.z) {
    return TurnAxis::Y;
  }

  return TurnAxis::Z;
}

int layerForAxis(const glm::vec3 &position, TurnAxis axis) {
  // The clicked cubie tells us which layer to turn
  // if axis is x we turn the clicked cubies x layer
  // if axis is y we turn the clicked cubies y layer
  // if axis is z we turn the clicked cubies z layer

  if (axis == TurnAxis::X) {
    return static_cast<int>(position.x);
  }
  if (axis == TurnAxis::Y) {
    return static_cast<int>(position.y);
  }
  if (axis == TurnAxis::Z) {
    return static_cast<int>(position.z);
  }

  return 0;
}

float directionSignForAxis(const glm::vec3 &direction, TurnAxis axis) {
  // a drag direction may point along the positive or negative side of the
  // chosen world axis we return +1 or -1 so the animation knows which way to
  // rotate
  float value = direction.x;

  if (axis == TurnAxis::Y) {
    value = direction.y;
  } else if (axis == TurnAxis::Z) {
    value = direction.z;
  }

  return value >= 0.0F ? 1.0F : -1.0F;
}

void Application::processMouseInput(const glm::mat4 &view,
                                    const glm::mat4 &projection, int displayW,
                                    int displayH) {
  double mouseX = 0.0;
  double mouseY = 0.0;
  glfwGetCursorPos(window_, &mouseX, &mouseY);

  const bool leftMouseDown =
      glfwGetMouseButton(window_, GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

  if (leftMouseDown && !isDraggingCamera_ && !isDraggingCube_) {
    activePick_ =
        pickCubie(mouseX, mouseY, view, projection, displayW, displayH);

    if (activePick_.hit) {
      // The drag started on a cubie, so this drag belongs to the cube.
      //
      // We do not immediately turn. We wait until the mouse moves far enough
      // to tell whether the drag is mostly horizontal or mostly vertical.
      isDraggingCube_ = true;
      hasStartedMouseTurn_ = false;
      dragStartX_ = mouseX;
      dragStartY_ = mouseY;
    } else {
      // The drag started in empty space, so this drag orbits the camera.
      isDraggingCamera_ = true;
      lastMouseX_ = mouseX;
      lastMouseY_ = mouseY;
    }
  }

  if (leftMouseDown && isDraggingCamera_) {
    const double dx = mouseX - lastMouseX_;
    const double dy = mouseY - lastMouseY_;
    const float mouseSensitivity = 0.005F;

    cameraYaw_ += static_cast<float>(dx) * mouseSensitivity;
    cameraPitch_ -= static_cast<float>(dy) * mouseSensitivity;

    lastMouseX_ = mouseX;
    lastMouseY_ = mouseY;
  }

  if (leftMouseDown && isDraggingCube_ && !hasStartedMouseTurn_ &&
      !isTurning_) {
    // Mouse movement since the cubie was clicked.
    const double dx = mouseX - dragStartX_;
    const double dy = mouseY - dragStartY_;

    // Do not start a turn from tiny mouse movement.
    // This prevents accidental turns from a small hand shake.
    const double dragDistance = std::sqrt(dx * dx + dy * dy);
    const double dragThreshold = 8.0;

    if (dragDistance >= dragThreshold) {
      // Decide whether this gesture is mostly horizontal or mostly vertical.
      //
      // Horizontal drag should feel like "move this row left/right."
      // Vertical drag should feel like "move this column up/down."
      const bool horizontalDrag = std::abs(dx) > std::abs(dy);

      // Convert the 2D screen drag into a 3D world direction.
      //
      // If the user dragged horizontally:
      //   use cameraRight_, with sign based on left/right.
      //
      // If the user dragged vertically:
      //   use cameraUp_, with sign based on up/down.
      //
      // Important:
      // Window y coordinates increase downward.
      // So dy < 0 means the mouse moved up.
      glm::vec3 worldDragDirection(0.0F);

      if (horizontalDrag) {
        worldDragDirection =
            static_cast<float>(dx >= 0.0 ? 1.0 : -1.0) * cameraRight_;
      } else {
        worldDragDirection =
            static_cast<float>(dy <= 0.0 ? 1.0 : -1.0) * cameraUp_;
      }

      // The mouse drag tells us which way the clicked face should visibly
      // move.
      //
      // But a Rubik's cube layer does not rotate around the drag direction.
      // It rotates around an axis that is perpendicular to:
      // 1. the clicked face normal
      // 2. the desired drag direction on that face
      //
      // Example:
      // If the user drags right on the front face, the front face should move
      // right. The turn axis must be vertical, not horizontal.
      //
      // This is the key relationship:
      // turn axis = clicked face normal x drag direction
      const glm::vec3 faceNormal = activePick_.faceNormal;

      // Project the drag onto the clicked face.
      //
      // cameraRight_ and cameraUp_ are screen directions in world space.
      // Depending on camera angle, part of that direction may point into or out
      // of the clicked face.
      //
      // A sticker can only slide along its own face, so we remove the part of
      // the drag that points through the face normal.
      glm::vec3 faceDragDirection =
          worldDragDirection -
          faceNormal * glm::dot(worldDragDirection, faceNormal);

      // If the camera is looking almost straight at an edge case, the projected
      // drag can become too small to trust. In that rare case, cancel this
      // drag instead of starting a wrong turn.
      if (glm::length(faceDragDirection) < 0.001F) {
        hasStartedMouseTurn_ = true;
        return;
      }

      faceDragDirection = glm::normalize(faceDragDirection);

      // Snap the face drag to one clean world axis.
      //
      // We do this because Rubik's cube turns are axis-aligned.
      // The mouse drag may be a little diagonal, but the final turn should be a
      // clean row/column movement.
      const TurnAxis dragAxis = strongAxisFromVector(faceDragDirection);
      glm::vec3 snappedDragDirection(0.0F);

      if (dragAxis == TurnAxis::X) {
        snappedDragDirection.x = faceDragDirection.x >= 0.0F ? 1.0F : -1.0F;
      } else if (dragAxis == TurnAxis::Y) {
        snappedDragDirection.y = faceDragDirection.y >= 0.0F ? 1.0F : -1.0F;
      } else {
        snappedDragDirection.z = faceDragDirection.z >= 0.0F ? 1.0F : -1.0F;
      }

      // This is the actual rotation axis.
      //
      // If this line feels confusing, remember:
      // - snappedDragDirection is where the clicked sticker should move
      // - faceNormal is which side of the cubie was clicked
      // - cross(faceNormal, snappedDragDirection) gives the spin axis that
      //   makes that face move in that drag direction
      const glm::vec3 turnAxisDirection =
          glm::normalize(glm::cross(faceNormal, snappedDragDirection));

      const TurnAxis turnAxis = strongAxisFromVector(turnAxisDirection);

      // Build a generic Move from the mouse drag.
      //
      // This is the same kind of Move the keyboard now creates.
      // The source is different, but the animation system receives the same
      // clean instruction.
      const Move dragMove{
          turnAxis,
          layerForAxis(activePick_.cubiePosition, turnAxis),
          directionSignForAxis(turnAxisDirection, turnAxis),
      };

      startTurn(dragMove);

      // Prevent this same drag from starting multiple turns.
      hasStartedMouseTurn_ = true;
    }
  }

  if (!leftMouseDown) {
    isDraggingCamera_ = false;
    isDraggingCube_ = false;
    hasStartedMouseTurn_ = false;
  }
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

    // build camera basis vectors for this frame
    // cameraForward points from the camera toward the cube
    // cameraRight points to the right side of the screen
    // cameraUp points to the top side of the screen
    // We store cameraRight_ and cameraUp_ so mouse drag code can convert
    // screen-space drag into world space turn directions
    const glm::vec3 cameraForward =
        glm::normalize(glm::vec3(0.0F, 0.0F, 0.0F) - cameraPosition);

    cameraRight_ =
        glm::normalize(glm::cross(cameraForward, glm::vec3(0.0F, 1.0F, 0.0F)));
    cameraUp_ = glm::normalize(glm::cross(cameraRight_, cameraForward));

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

    processMouseInput(view, projection, displayW, displayH);

    // update which face is currwently under the mouse
    // we only show hoever when the user is not dragging and when a turen is not
    // animating this keeps hover feedback simple:
    // - move mouse over cube: face highlights
    // click/drag cube: hover disappears and drag and drag logic takes over
    // turn anumation: hover disappears until the turn finishes
    if (!isDraggingCamera_ && !isDraggingCube_ && !isTurning_) {
      double mouseX = 0.0;
      double mouseY = 0.0;
      glfwGetCursorPos(window_, &mouseX, &mouseY);

      hoverPick_ =
          pickCubie(mouseX, mouseY, view, projection, displayW, displayH);
    } else {
      hoverPick_ = MousePick{};
    }

    if (viewLoc_ != -1) {
      glUniformMatrix4fv(viewLoc_, 1, GL_FALSE, glm::value_ptr(view));
    }
    if (projectionLoc_ != -1) {
      glUniformMatrix4fv(projectionLoc_, 1, GL_FALSE,
                         glm::value_ptr(projection));
    }

    // Centers are close now, so the Rubik's cube looks connected.
    //
    // We still leave a tiny gap between cubies.
    // That gap reveals the dark/internal faces and looks like black separation
    // lines.
    const float cubieSpacing = 0.50F;

    // Colored cubie size.
    // Slightly smaller than spacing so black gaps remain visible.
    const float cubieScale = 0.46F;

    for (const Cubie &cubie : cubies_) {
      // Decide whether this specific cubie should receive the layer rotation.
      //
      // A cubie should rotate if:
      // 1. a turn angle is currently visible
      // 2. it belongs to the moving axis/layer
      //
      // This works for both:
      // - keyboard fallback turns on outer faces
      // - mouse drag turns on rows, columns, and depth layers
      const bool cubieIsTurning =
          turnAngle_ > 0.0F && isCubieInTurningLayer(cubie);

      // layerRotation is either:
      // - a real rotation matrix for cubies in the turning layer
      // - identity matrix for cubies outside the turning layer
      //
      // Identity matrix means "do nothing."
      const glm::mat4 layerRotation =
          cubieIsTurning
              ? glm::rotate(glm::mat4(1.0F), turnDirection_ * turnAngle_,
                            rotationAxisForTurnAxis(turningAxis_))
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

      glUniformMatrix4fv(modelLoc_, 1, GL_FALSE, glm::value_ptr(model));

      // Draw normal sticker colors.
      //
      // useOverrideColor was used by the failed "draw bigger black cube first"
      // outline experiment. We keep this set to 0 so the shader uses each
      // cubie's sticker colors instead of solid black.
      glUniform1i(useOverrideColorLoc_, 0);

      // tell the shader which cubie is being drawn right nbow
      // the shader recieves this once per cubie
      // then it can compare this cubie against hoverPick_.cubiePosition
      glUniform3fv(cubiePositionLoc_, 1, glm::value_ptr(cubie.position));

      // tell the shader whether hover highlighting should be active
      glUniform1i(highlightEnabledLoc_, hoverPick_.hit ? 1 : 0);

      // send the hovered cubie's grid position
      glUniform3fv(highlightedCubiePositionLoc_, 1,
                   glm::value_ptr(hoverPick_.cubiePosition));

      // send the hovered face direction
      // this is what makes only one face highlight instead of the whole cubie
      glUniform3fv(highlightedFaceNormalLoc_, 1,
                   glm::value_ptr(hoverPick_.faceNormal));

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

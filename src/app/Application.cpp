// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <ios>
#include <iostream>
#include <limits>
#include <sstream>
#include <stdexcept>
// give this source file access rto glfw functions
#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <imgui.h>
#include <imgui_impl_glfw.h>
#include <imgui_impl_opengl3.h>

// basic glm vector and matrix types
#include <glm/glm.hpp>
// glm::rotate, glm::translate, glm::perspective
#include <glm/gtc/matrix_transform.hpp>

// glm::value_ptr for sending matrices to openGL
#include <cmath>
#include <glm/gtc/type_ptr.hpp>
#include <random>

// anonymous namesce means these helper functions are only visible inside this
// .cpp file
namespace {
bool sameColor(const glm::vec3 &a, const glm::vec3 &b) {
  // Colors are stored as floats.
  //
  // Our sticker colors are assigned directly, so exact equality would probably
  // work here. A small tolerance is still a better habit in graphics code.
  const float epsilon = 0.001F;

  return std::abs(a.x - b.x) < epsilon && std::abs(a.y - b.y) < epsilon &&
         std::abs(a.z - b.z) < epsilon;
}

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

  // set up dear imgui - draws pratcical tool ui oover our openGL scene
  // we use it for buttons states controls and later timer controls
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();

  ImGuiIO &io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui::StyleColorsDark();

  ImGui_ImplGlfw_InitForOpenGL(window_, true);
  ImGui_ImplOpenGL3_Init("#version 330");
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
  moveQueue_.clear();
  moveHistory_.clear();
  latestScrambleMoves_.clear();
  hasLastRecordedMove_ = false;
  currentMove_ = Move{TurnAxis::X, 1, 1.0F};
  recordCurrentMoveInHistory_ = true;

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

bool Application::isCubeSolved() const {
  // Solved face colors.
  //
  // These match the colors assigned in resetCubeState().
  const glm::vec3 green(0.0F, 0.8F, 0.1F);
  const glm::vec3 blue(0.0F, 0.2F, 1.0F);
  const glm::vec3 orange(1.0F, 0.45F, 0.0F);
  const glm::vec3 red(0.9F, 0.0F, 0.0F);
  const glm::vec3 white(1.0F, 1.0F, 1.0F);
  const glm::vec3 yellow(1.0F, 0.9F, 0.0F);

  for (const Cubie &cubie : cubies_) {
    // Front side of the whole cube.
    //
    // If a cubie is on z = +1, its outward front sticker should be green.
    if (cubie.position.z == 1.0F && !sameColor(cubie.frontColor, green)) {
      return false;
    }

    // Back side of the whole cube.
    if (cubie.position.z == -1.0F && !sameColor(cubie.backColor, blue)) {
      return false;
    }

    // Left side of the whole cube.
    if (cubie.position.x == -1.0F && !sameColor(cubie.leftColor, orange)) {
      return false;
    }

    // Right side of the whole cube.
    if (cubie.position.x == 1.0F && !sameColor(cubie.rightColor, red)) {
      return false;
    }

    // Top side of the whole cube.
    if (cubie.position.y == 1.0F && !sameColor(cubie.topColor, white)) {
      return false;
    }

    // Bottom side of the whole cube.
    if (cubie.position.y == -1.0F && !sameColor(cubie.bottomColor, yellow)) {
      return false;
    }
  }

  return true;
}

Application::~Application() {
  ImGui_ImplOpenGL3_Shutdown();
  ImGui_ImplGlfw_Shutdown();
  ImGui::DestroyContext();

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

  // Backspace hard-resets the cube to solved state.
  //
  // We check for "pressed now" and "was not pressed last frame" so one key
  // press creates one reset.
  const bool resetIsPressed =
      glfwGetKey(window_, GLFW_KEY_BACKSPACE) == GLFW_PRESS;

  if (resetIsPressed && !resetWasPressed_) {
    resetCubeState();
  }
  resetWasPressed_ = resetIsPressed;

  // Return toggles the timer.
  //
  // This is separate from cube reset now:
  // - Return: start/stop timer
  // - Backspace: hard reset cube
  const bool timerToggleIsPressed =
      glfwGetKey(window_, GLFW_KEY_ENTER) == GLFW_PRESS;

  if (timerToggleIsPressed && !timerToggleWasPressed_) {
    toggleTimer();
  }
  timerToggleWasPressed_ = timerToggleIsPressed;

  // X starts a scramble.
  const bool scrambleIsPressed = glfwGetKey(window_, GLFW_KEY_X) == GLFW_PRESS;

  if (scrambleIsPressed && !scrambleWasPressed_) {
    scrambleCube();
  }
  scrambleWasPressed_ = scrambleIsPressed;

  // V performs an animated reset by undoing every recorded move.
  const bool solveBackIsPressed = glfwGetKey(window_, GLFW_KEY_V) == GLFW_PRESS;

  if (solveBackIsPressed && !solveBackWasPressed_) {
    solveBackFromMoveHistory();
  }
  solveBackWasPressed_ = solveBackIsPressed;
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
    const float turnSpeed = targetAngle / currentTurnDurationSeconds_;

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

  // Stop the timer automatically when the cube becomes solved.
  //
  // We only check while the cube is fully idle:
  // - no active turn animation
  // - no queued sequence still waiting
  //
  // That prevents the timer from stopping in the middle of an animated reset
  // or scramble sequence.
  if (!isTurning_ && moveQueue_.empty() && timerRunning_ && isCubeSolved()) {
    stopTimer();
  }

  // if the cube is idle and moves are wating start the next queued move
  // this is what makes scramble and solve back anumate move by move
  updateMoveQueue();

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

void Application::startTurn(const Move &move, bool recordInHistory,
                            float turnDurationSeconds) {
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
  currentMove_ = move;
  recordCurrentMoveInHistory_ = recordInHistory;

  // Protect the animation math from zero or extremely tiny durations.
  //
  // Normal turns use the default 0.5 seconds.
  // Animated reset can pass smaller values when there are many moves to undo.
  if (turnDurationSeconds < 0.008F) {
    currentTurnDurationSeconds_ = 0.008F;
  } else {
    currentTurnDurationSeconds_ = turnDurationSeconds;
  }

  // Start the visual animation from 0 degrees.
  isTurning_ = true;
  turnAngle_ = 0.0F;
}

void Application::queueMove(const Move &move, bool recordInHistory,
                            float turnDurationSeconds) {
  // add one move to the back of the waiting list
  moveQueue_.push_back(QueuedMove{move, recordInHistory, turnDurationSeconds});
}

void Application::queueMoves(const std::vector<Move> &moves) {
  // add each move in order
  // we dont start them all at once
  // updateMoveQueue() will play them one at a time
  for (const Move &move : moves) {
    queueMove(move);
  }
}

void Application::updateMoveQueue() {
  // if a move is already animating then wait
  if (isTurning_) {
    return;
  }
  // if no moves are waiting, there is nothing to do
  if (moveQueue_.empty()) {
    return;
  }

  // copy the next move, then remove it from the queue
  // erase(begin) rmeoves the fist item
  // for a tiny rubik scramble queue this is fine
  // later, std::deque would be better for large queues
  const QueuedMove nextMove = moveQueue_.front();
  moveQueue_.erase(moveQueue_.begin());

  startTurn(nextMove.move, nextMove.recordInHistory,
            nextMove.turnDurationSeconds);
}

Move Application::inverseMove(const Move &move) const {
  // same axis, same layer, oppposite direction
  return Move{
      move.axis,
      move.layer,
      -move.direction,
  };
}

std::string Application::moveToNotation(const Move &move) const {
  // Standard Rubik notation names the outer faces:
  //
  // X axis:
  //   layer  1 -> R
  //   layer -1 -> L
  //
  // Y axis:
  //   layer  1 -> U
  //   layer -1 -> D
  //
  // Z axis:
  //   layer  1 -> F
  //   layer -1 -> B
  //
  // The direction sign decides whether we add a prime mark.
  // In this project, the "normal" direction matches the layer sign:
  // +1 layer with +1 direction is R/U/F.
  // -1 layer with -1 direction is L/D/B.
  std::string notation = "?";

  if (move.axis == TurnAxis::X) {
    notation = move.layer >= 0 ? "R" : "L";
  } else if (move.axis == TurnAxis::Y) {
    notation = move.layer >= 0 ? "U" : "D";
  } else {
    notation = move.layer >= 0 ? "F" : "B";
  }

  const bool isPrimeMove =
      (move.layer >= 0 && move.direction < 0.0F) ||
      (move.layer < 0 && move.direction > 0.0F);

  if (isPrimeMove) {
    notation += "'";
  }

  return notation;
}

std::string Application::movesToNotation(const std::vector<Move> &moves) const {
  std::ostringstream stream;

  for (std::size_t i = 0; i < moves.size(); ++i) {
    if (i > 0) {
      stream << ' ';
    }

    stream << moveToNotation(moves[i]);
  }

  return stream.str();
}

void Application::scrambleCube() {
  // dont start a new scramble while a turn or queue is already active
  // this keeps state predictable while we are learning
  if (isTurning_ || !moveQueue_.empty()) {
    return;
  }

  latestScrambleMoves_.clear();

  // random number tools
  // rabdom_device is the actual random generator
  static std::random_device randomDevice;
  static std::mt19937 generator(randomDevice());

  // Axis choices:
  //  0 -> x
  //  1 -> y
  //  2 -> z
  std::uniform_int_distribution<int> axisDistribution(0, 2);
  std::uniform_int_distribution<int> layerDistribution(0, 1);

  // direction choices:
  // 0 -> -1
  // 1 -> +1
  std::uniform_int_distribution<int> directionDistribution(0, 1);

  const int scrambleLength = 20;
  bool hasPreviousMove = false;
  Move previousMove{TurnAxis::X, 1, 1.0F};

  for (int i = 0; i < scrambleLength; ++i) {
    Move move{TurnAxis::X, 1, 1.0F};

    while (true) {
      const int axisValue = axisDistribution(generator);
      const int layerValue = layerDistribution(generator);
      const int directionValue = directionDistribution(generator);

      TurnAxis axis = TurnAxis::X;
      if (axisValue == 1) {
        axis = TurnAxis::Y;
      } else if (axisValue == 2) {
        axis = TurnAxis::Z;
      }

      const int layer = layerValue == 0 ? -1 : 1;
      const float direction = directionValue == 0 ? -1.0F : 1.0F;

      move = Move{axis, layer, direction};

      // Avoid weak scramble moves.
      //
      // If we allow the same axis twice in a row, we can get sequences like:
      // R then R'
      // R then L
      // U then D
      //
      // Some are direct cancellations, and some are just less useful for a
      // beginner scramble. Choosing a different axis each time keeps the
      // scramble more varied and easier to reason about.
      if (!hasPreviousMove || move.axis != previousMove.axis) {
        break;
      }
    }

    queueMove(move);
    latestScrambleMoves_.push_back(move);
    previousMove = move;
    hasPreviousMove = true;
  }
}

void Application::solveBackFromMoveHistory() {
  // do not solve back wgile another sequence is still running
  if (isTurning_ || !moveQueue_.empty()) {
    return;
  }

  const int moveCount = static_cast<int>(moveHistory_.size());
  if (moveCount == 0) {
    return;
  }

  // Maximum total time for animated reset.
  const float maxSolveBackSeconds = 4.0F;

  // Give every inverse move an equal slice of the time budget.
  //
  // A tiny minimum prevents huge histories from requesting near-zero duration
  // turns, which can look like flickering instead of animation.
  float secondsPerMove = maxSolveBackSeconds / static_cast<float>(moveCount);
  if (secondsPerMove < 0.008F) {
    secondsPerMove = 0.008F;
  }

  // Use moveHistory_ like a stack.
  //
  // The latest move must be undone first.
  // Example:
  // history: A, B, C
  // solve-back queue: inverse(C), inverse(B), inverse(A)
  while (!moveHistory_.empty()) {
    const Move latestMove = moveHistory_.back();
    moveHistory_.pop_back();

    // These inverse moves are intentionally not recorded.
    //
    // Otherwise solve-back would refill the history with inverse moves while it
    // is trying to empty the history.
    queueMove(inverseMove(latestMove), false, secondsPerMove);
  }
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

  // Record the move after it has successfully changed the cube state.
  //
  // This makes moveHistory_ a true stack of completed moves.
  //
  // Normal moves record here:
  // - mouse drag turns
  // - scramble turns
  //
  // Solve-back inverse moves do not record here, because startTurn() marks
  // them with recordCurrentMoveInHistory_ = false.
  if (recordCurrentMoveInHistory_) {
    moveHistory_.push_back(currentMove_);
    lastRecordedMove_ = currentMove_;
    hasLastRecordedMove_ = true;
  }
}

int Application::run() {
  std::cout << "RubikSim starting...\n";
  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    ImGui_ImplOpenGL3_NewFrame();
    ImGui_ImplGlfw_NewFrame();
    ImGui::NewFrame();

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

    // if imgui wnats the mouse the use is clicking/draggin the
    // uniform_int_distributionin that case do not also rortate the cube/camera
    if (!ImGui::GetIO().WantCaptureMouse) {
      processMouseInput(view, projection, displayW, displayH);
    }

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

    renderUi();

    ImGui::Render();
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

    glfwSwapBuffers(window_);
  }
  return 0;
}
} // namespace rubiksim

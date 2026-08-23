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

  // Disable vsync while we are tuning mouse controls.
  //
  // Vsync waits for the monitor before presenting each frame. That can make
  // dragging feel delayed because the mouse input is ready before the screen is
  // allowed to update.
  //
  // Tradeoff:
  // - 0: lower input lag
  // - 1: less screen tearing
  glfwSwapInterval(0);

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

    // Camera mouse movement must be applied before we build the camera
    // matrices below.
    //
    // If we update cameraYaw_ / cameraPitch_ after creating view, the rendered
    // camera will always be one frame behind the mouse.
    processCameraMouseInput(!ImGui::GetIO().WantCaptureMouse);

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
      processMouseInput();
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

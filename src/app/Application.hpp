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
#include <string>
#include <vector>

namespace rubiksim {
struct Cubie {
  // grid pos inisde the cube
  glm::vec3 position;
  glm::vec3 frontColor;
  glm::vec3 backColor;
  glm::vec3 leftColor;
  glm::vec3 rightColor;
  glm::vec3 topColor;
  glm::vec3 bottomColor;
};

// A turn can happen around one of the three world axes.
//
// X axis turns left/right layers.
// Y axis turns top/bottom layers.
// Z axis turns front/back layers.
enum class TurnAxis { X, Y, Z };

// A Move describes one 90-degree layer turn.
//
// It does not care whether the move came from:
// - keyboard input
// - mouse drag
// - scramble generator
// - UI button
//
// Everything becomes the same simple instruction:
// rotate this axis, on this layer, in this direction.
struct Move {
  // Which world axis the layer rotates around.
  TurnAxis axis;

  // Which layer on that axis moves.
  //
  // For a 3x3 cube:
  // -1 means negative side
  //  0 means middle layer
  //  1 means positive side
  int layer;

  // +1 means rotate in the positive axis direction.
  // -1 means rotate in the negative axis direction.
  float direction;
};

struct QueuedMove {
  Move move;
  bool recordInHistory{true};
  float turnDurationSeconds{0.5F};
};

struct MousePick {
  // hit is false when the mouse ray does not touch any cubie.
  bool hit{false};

  // Grid position of the cubie that was clicked.
  glm::vec3 cubiePosition{0.0F};

  // Direction of the clicked face.
  // Examples:
  // front face = (0, 0, 1)
  // right face = (1, 0, 0)
  // top face = (0, 1, 0)
  glm::vec3 faceNormal{0.0F};

  // Distance from the camera ray origin to the hit.
  // If several cubies are under the mouse, we keep the closest one.
  float distance{0.0F};
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
  void processCameraMouseInput(bool allowStartingNewDrag);
  void processMouseInput();

  MousePick pickCubie(double mouseX, double mouseY, const glm::mat4 &view,
                      const glm::mat4 &projection, int displayW,
                      int displayH) const;

  bool isCubieInTurningLayer(const Cubie &cubie) const;

  // Rebuilds the cube into the solved state.
  //
  // This resets:
  // - every cubie's grid position
  // - every sticker color
  // - any active turn animation
  //
  // Later the UI reset button can call this same function.
  void resetCubeState();

  // Returns true when every visible sticker is back on its solved face.
  //
  // This checks sticker colors, not move history.
  // That means it still works if the user solves the cube manually.
  bool isCubeSolved() const;

  // Starts a turn animation from a Move.
  //
  // This becomes the one official way to start a cube rotation.
  // Keyboard, mouse, scramble, and UI will all call this later.
  void startTurn(const Move &move, bool recordInHistory = true,
                 float turnDurationSeconds = 0.5F);

  // Permanently applies the finished 90 degree turn to cubie data.
  //
  // The animation only changes how cubies are drawn.
  // This function changes the actual stored cubie.position values and sticker
  // colors so the cube remembers the move.
  void applyTurnToCubeState();

  // adds one move to the waiting list
  // if no turn is currently active, updateMoveQueue() will start it
  void queueMove(const Move &move, bool recordInHistory = true,
                 float turnDurationSeconds = 0.5F);

  // adds several moves to the waiting list
  // scramble and solve back will use this
  void queueMoves(const std::vector<Move> &moves);

  // starts the next queued move if the cube is idle
  void updateMoveQueue();

  // Creates a random scramble and queues it
  void scrambleCube();

  // Queues inverse moves from the move history stack.
  //
  // We use this as the animated reset:
  // it walks backward through every completed move and undoes them one by one.
  void solveBackFromMoveHistory();

  // returns the opposite of a move
  // if a move rotates 90 degrees its inverse rotates -90 degrees
  Move inverseMove(const Move &move) const;

  // Converts a Move into standard-looking Rubik notation.
  //
  // Examples:
  // axis X, layer  1, direction  1 -> R
  // axis X, layer  1, direction -1 -> R'
  // axis Y, layer  1, direction  1 -> U
  std::string moveToNotation(const Move &move) const;

  // Converts a list of moves into one readable line.
  //
  // Example:
  // R U F' L
  std::string movesToNotation(const std::vector<Move> &moves) const;

  // Duration of the move currently being animated.
  //
  // Normal moves use about 0.5 seconds.
  // Animated reset moves calculate a duration from the history stack size.
  float currentTurnDurationSeconds_{0.5F};

  // draws the imgui control panel
  // this ui calls the same fucntions as the keyboard shortcuts
  // that means buttons do not ceate sep behaviour
  void renderUi();

  // Starts the solve timer if it is not already running.
  //
  // We call this from:
  // - the UI Start button
  // - the Return key
  // - the first manual cube rotation
  void startTimer();

  // Stops the solve timer but keeps the elapsed time visible.
  void stopTimer();

  // Switches between start and stop.
  void toggleTimer();

  // Clears the timer back to 0.
  void resetTimer();

  // Returns the timer value that should be displayed.
  double elapsedTimerSeconds() const;

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
  // int cubiePositionLoc_{-1};
  int frontColorLoc_{-1};
  int backColorLoc_{-1};
  int leftColorLoc_{-1};
  int rightColorLoc_{-1};
  int topColorLoc_{-1};
  int bottomColorLoc_{-1};

  int useOverrideColorLoc_{-1};
  int overrideColorLoc_{-1};

  // sends the current cubies grid position to the shaderProgram_
  // example:\
  // left op front cubue position is (-1,1,1)
  // the shader uses this to know whether the cubie being drawn is the same
  // cubie thayt the mouse is hovering over
  int cubiePositionLoc_{-1};

  // Tells the shader whether hover highlighting is active this frame
  // 0 means no hover
  // 1 means brighten the hovered faceNormal
  int highlightEnabledLoc_{-1};
  int highlightedCubiePositionLoc_{-1};

  // the face direction currently undser the mouseX
  // front face = (0,0,1)
  // right face (1,0,0)
  // top face = (0,0,0)
  int highlightedFaceNormalLoc_{-1};

  // camera orbit values
  // yaw rotates left/right around the cubes
  // pitch rotates up/down
  // distance controls zoom
  float cameraYaw_{0.8F};
  float cameraPitch_{0.5F};
  float cameraDistance_{6.0F};

  // camera basis vectors for the current 15:54
  // cameraRight_ points to the right side of the screen in world space
  // cameraUp_ points to the top of the screen in world space
  // we use these to convert a 2d mouse frag into a 3d cube turn
  //
  // Example
  // if the user drags horizontally we compare that gesture with cameraRight_
  // if the user drags vertically we compare that gesture with cameraUp_
  glm::vec3 cameraRight_{1.0F, 0.0F, 0.0F};
  glm::vec3 cameraUp_{0.0F, 1.0F, 0.0F};

  // Mouse camera-drag state.
  //
  // When the left mouse button is held down, we compare the current mouse
  // position with the previous mouse position.
  // The difference is called mouse delta.
  //
  // mouse delta x changes cameraYaw_.
  // mouse delta y changes cameraPitch_.
  bool isDraggingCamera_{false};
  double lastMouseX_{0.0};
  double lastMouseY_{0.0};

  // Mouse cube-drag state.
  //
  // A mouse drag can mean two different things:
  // 1. drag empty space: orbit the camera
  // 2. drag from a cubie: turn a row/column/layer
  //
  // activePick_ stores what cubie face the drag started on.
  bool isDraggingCube_{false};
  bool hasStartedMouseTurn_{false};
  double dragStartX_{0.0};
  double dragStartY_{0.0};
  MousePick activePick_;

  // stores the cubie face currently under the mouseX
  // this is seperate from activePick_
  // activePick_ means - where did the click drag start?
  // hoverPick_ means - what face is the mouse over right now
  MousePick hoverPick_;

  // used to calc deltaTime
  float lastFrameTime_{0.0F};
  // stores all small cubes that make up the rubik's cubeMesh_
  std::vector<Cubie> cubies_;

  // moves waiting to be animated
  // cube can only animate one move at a time
  // so we store upcoming mobves here and play them one by once
  std::vector<QueuedMove> moveQueue_;

  // Stack of moves that have actually finished.
  //
  // This records both:
  // - mouse drag moves
  // - scramble moves
  //
  // We use it as a stack:
  // - push_back() records a finished move
  // - back() reads the latest move
  // - pop_back() removes the latest move
  std::vector<Move> moveHistory_;

  // Latest scramble sequence generated by X or the Scramble button.
  //
  // This is display-only. It lets the UI show the scramble text even while the
  // move queue is playing it.
  std::vector<Move> latestScrambleMoves_;

  // Last move that finished and was recorded into moveHistory_.
  Move lastRecordedMove_{TurnAxis::X, 1, 1.0F};
  bool hasLastRecordedMove_{false};

  // key guards so holding a keu does not trigger the action every frame
  bool scrambleWasPressed_{false};
  bool solveBackWasPressed_{false};

  // Tracks whether Return was already down last frame.
  //
  // Return toggles the timer. This guard makes one key press create one
  // start/stop action.
  bool timerToggleWasPressed_{false};

  // Timer state.
  //
  // timerRunning_ tells us whether time is actively counting right now.
  // timerStartTime_ stores when the current running segment started.
  // accumulatedTimerSeconds_ stores time from previous running segments.
  bool timerRunning_{false};
  double timerStartTime_{0.0};
  double accumulatedTimerSeconds_{0.0};

  // Face-turn animation state.
  //
  // These variables describe a turn that is happening right now.
  //
  // Example:
  // You drag one cubie face with the mouse.
  // startTurn() stores the axis/layer/direction.
  // turnAngle_ starts at 0.
  // Every frame, turnAngle_ increases until it reaches 90 degrees.
  bool isTurning_{false};

  // Tracks whether the reset key was already down last frame.
  //
  // This is an edge-trigger guard:
  // holding the reset key should reset once, not every frame.
  bool resetWasPressed_{false};

  // Generic turn description.
  //
  // turningAxis_ tells us which axis the layer spins around.
  // turningLayer_ tells us which layer on that axis moves. It is -1, 0, or 1.
  // turnDirection_ is +1 or -1 and controls clockwise/counter-clockwise.
  TurnAxis turningAxis_{TurnAxis::X};
  int turningLayer_{1};
  float turnDirection_{1.0F};

  // The move currently being animated.
  //
  // We store the full Move so applyTurnToCubeState() can push it onto
  // moveHistory_ when the animation successfully finishes.
  Move currentMove_{TurnAxis::X, 1, 1.0F};

  // Some moves should not be recorded.
  //
  // Example:
  // solve-back plays inverse moves from history.
  // If we recorded those inverse moves too, the stack would refill itself while
  // solving back.
  bool recordCurrentMoveInHistory_{true};

  // Current animation angle in radians.
  //
  // GLM rotation functions use radians, not degrees.
  // 0 radians means no rotation.
  // glm::radians(90.0F) means a quarter turn.
  float turnAngle_{0.0F};

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

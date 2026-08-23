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
  // grid pos inisde the cube
  glm::vec3 position;
  glm::vec3 frontColor;
  glm::vec3 backColor;
  glm::vec3 leftColor;
  glm::vec3 rightColor;
  glm::vec3 topColor;
  glm::vec3 bottomColor;
};

// enum means a variabkle that can only be one of these named choices
// so instead of remembering: 0 means right, 1 means keft, 2 means top :
// we can write readable code -> SelectedFace::Right, SelectedFace::Face
enum class SelectedFace { Right, Left, Top, Bottom, Front, Back };

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
  void processMouseInput(const glm::mat4 &view, const glm::mat4 &projection,
                         int displayW, int displayH);

  MousePick pickCubie(double mouseX, double mouseY, const glm::mat4 &view,
                      const glm::mat4 &projection, int displayW,
                      int displayH) const;

  // Returns true if this cubie belongs to the requested face.
  //
  // The const at the end means this function promises not to modify
  // Application.
  //
  // We pass the face in as an argument instead of always using selectedFace_.
  // Why?
  // Because selectedFace_ can change while an animation is running, but the
  // running animation should keep using the face that started the turn.
  bool isCubieInFace(const Cubie &cubie, SelectedFace face) const;
  bool isCubieInTurningLayer(const Cubie &cubie) const;

  // Starts a turn animation from a Move.
  //
  // This becomes the one official way to start a cube rotation.
  // Keyboard, mouse, scramble, and UI will all call this later.
  void startTurn(const Move &move);

  // Converts the currently selected keyboard face into a Move.
  //
  // Example:
  // SelectedFace::Right becomes:
  // axis X, layer 1, direction +1
  Move moveForSelectedFace(SelectedFace face) const;

  // Permanently applies the finished 90 degree turn to cubie data.
  //
  // The animation only changes how cubies are drawn.
  // This function changes the actual stored cubie.position values and sticker
  // colors so the cube remembers the move.
  void applyTurnToCubeState();

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
  // the face currently selected for a future move
  // we start with the right face bfecause is the first standard Rubik move
  SelectedFace selectedFace_{SelectedFace::Right};
  // stores all small cubes that make up the rubik's cubeMesh_
  std::vector<Cubie> cubies_;

  // Face-turn animation state.
  //
  // These variables describe a turn that is happening right now.
  //
  // Example:
  // You select the Right face.
  // You press Space.
  // isTurning_ becomes true.
  // turningFace_ stores Right.
  // turnAngle_ starts at 0.
  // Every frame, turnAngle_ increases until it reaches 90 degrees.
  bool isTurning_{false};

  // Tracks whether Space was already down last frame.
  // Without this, holding Space for a few frames could start the turn more than
  // once. We want one press to start one animation.
  bool spaceWasPressed_{false};

  // Stores which face is currently being animated.
  //
  // We copy selectedFace_ into turningFace_ when the animation starts.
  // Why not just use selectedFace_ directly?
  // Because the user might press another face key while the turn is animating.
  // The current animation should keep rotating the original face.
  SelectedFace turningFace_{SelectedFace::Right};

  // Generic turn description.
  //
  // turningAxis_ tells us which axis the layer spins around.
  // turningLayer_ tells us which layer on that axis moves. It is -1, 0, or 1.
  // turnDirection_ is +1 or -1 and controls clockwise/counter-clockwise.
  TurnAxis turningAxis_{TurnAxis::X};
  int turningLayer_{1};
  float turnDirection_{1.0F};

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

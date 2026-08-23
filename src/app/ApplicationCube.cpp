#include "Application.hpp"

#include <cmath>
#include <random>
#include <sstream>

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
} // namespace

namespace rubiksim {
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
} // namespace rubiksim

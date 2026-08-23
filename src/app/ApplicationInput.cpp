#include "Application.hpp"

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>

#include <cmath>
#include <limits>

#include <glm/gtc/matrix_transform.hpp>

namespace rubiksim {
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

  const float framebufferMouseX = static_cast<float>(mouseX) *
                                  static_cast<float>(displayW) /
                                  static_cast<float>(windowW);
  const float framebufferMouseY = static_cast<float>(mouseY) *
                                  static_cast<float>(displayH) /
                                  static_cast<float>(windowH);

  const glm::vec4 viewport(0.0F, 0.0F, static_cast<float>(displayW),
                           static_cast<float>(displayH));
  const glm::vec3 nearPoint = glm::unProject(
      glm::vec3(framebufferMouseX,
                static_cast<float>(displayH) - framebufferMouseY, 0.0F),
      view, projection, viewport);
  const glm::vec3 farPoint = glm::unProject(
      glm::vec3(framebufferMouseX,
                static_cast<float>(displayH) - framebufferMouseY, 1.0F),
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

    cameraYaw_ -= static_cast<float>(dx) * mouseSensitivity;
    cameraPitch_ += static_cast<float>(dy) * mouseSensitivity;

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

      // A manual cube rotation starts the timer automatically.
      //
      // Scramble and animated reset do not pass through this mouse-drag path,
      // so they do not auto-start the timer.
      startTimer();

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
} // namespace rubiksim

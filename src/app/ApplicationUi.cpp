#include "Application.hpp"

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <imgui.h>

namespace rubiksim {
void Application::startTimer() {
  if (timerRunning_) {
    return;
  }

  timerRunning_ = true;
  timerStartTime_ = glfwGetTime();
}

void Application::stopTimer() {
  if (!timerRunning_) {
    return;
  }

  accumulatedTimerSeconds_ += glfwGetTime() - timerStartTime_;
  timerRunning_ = false;
}

void Application::toggleTimer() {
  if (timerRunning_) {
    stopTimer();
  } else {
    startTimer();
  }
}

void Application::resetTimer() {
  timerRunning_ = false;
  timerStartTime_ = 0.0;
  accumulatedTimerSeconds_ = 0.0;
}

double Application::elapsedTimerSeconds() const {
  if (timerRunning_) {
    return accumulatedTimerSeconds_ + (glfwGetTime() - timerStartTime_);
  }

  return accumulatedTimerSeconds_;
}

void Application::renderUi() {
  // this creates ine small overlay window
  // later we can split this into controls, timer and dataset toldl
  ImGui::Begin("RubikSim Controls");

  // The cube is busy when a turn is animating or when queued moves are waiting.
  //
  // While busy, we disable cube action buttons so the user cannot start a
  // scramble/reset sequence on top of an existing sequence from the UI.
  const bool cubeBusy = isTurning_ || !moveQueue_.empty();
  const double elapsedSeconds = elapsedTimerSeconds();
  const int elapsedMinutes = static_cast<int>(elapsedSeconds / 60.0);
  const double remainingSeconds =
      elapsedSeconds - static_cast<double>(elapsedMinutes) * 60.0;

  ImGui::Text("Controls");
  ImGui::Separator();
  ImGui::Text("Mouse drag cubie: rotate layer");
  ImGui::Text("Mouse drag empty space: orbit camera");
  ImGui::Text("W / S: zoom");
  ImGui::Text("X: scramble");
  ImGui::Text("V: animated reset");
  ImGui::Text("Return: start/stop timer");
  ImGui::Text("Backspace: hard reset");

  ImGui::Spacing();
  ImGui::Separator();

  ImGui::Text("Timer");
  ImGui::Text("%02d:%05.2f", elapsedMinutes, remainingSeconds);
  ImGui::Text("Timer status: %s", timerRunning_ ? "running" : "stopped");

  if (ImGui::Button(timerRunning_ ? "Stop Timer" : "Start Timer")) {
    toggleTimer();
  }

  ImGui::SameLine();

  if (ImGui::Button("Reset Timer")) {
    resetTimer();
  }

  ImGui::Spacing();
  ImGui::Separator();

  ImGui::Text("State");
  ImGui::Text("Move history: %zu", moveHistory_.size());
  ImGui::Text("Move queue: %zu", moveQueue_.size());
  ImGui::Text("Turning: %s", isTurning_ ? "yes" : "no");
  ImGui::Text("Busy: %s", cubeBusy ? "yes" : "no");
  ImGui::Text("Solved: %s", isCubeSolved() ? "yes" : "no");
  ImGui::Text("Animated reset moves: %zu", moveHistory_.size());
  const std::string lastMoveNotation =
      hasLastRecordedMove_ ? moveToNotation(lastRecordedMove_) : "none";
  ImGui::Text("Last move: %s", lastMoveNotation.c_str());

  if (latestScrambleMoves_.empty()) {
    ImGui::TextWrapped("Latest scramble: none");
  } else {
    const std::string scrambleNotation =
        movesToNotation(latestScrambleMoves_);
    ImGui::TextWrapped("Latest scramble: %s", scrambleNotation.c_str());
  }

  ImGui::Spacing();
  ImGui::Separator();

  ImGui::BeginDisabled(cubeBusy);
  if (ImGui::Button("Scramble")) {
    scrambleCube();
  }
  ImGui::EndDisabled();

  ImGui::SameLine();

  ImGui::BeginDisabled(cubeBusy || moveHistory_.empty());
  if (ImGui::Button("Animated Reset")) {
    solveBackFromMoveHistory();
  }
  ImGui::EndDisabled();

  ImGui::BeginDisabled(cubeBusy);
  if (ImGui::Button("Hard Reset")) {
    resetCubeState();
  }
  ImGui::EndDisabled();

  ImGui::End();
}
} // namespace rubiksim

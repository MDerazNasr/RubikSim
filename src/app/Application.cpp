// includes your header file
// #include "app/Application.hpp"
#include "Application.hpp"
// this includes the standard linrary stream tools
// Angle brackets are used here
#include <iostream>

namespace rubiksim {
// means this funct belongs to the application class
int Application::run() const {
  std::cout << "RubikSim starting...\n";
  return 0; // returns upon success
}

} // namespace rubiksim

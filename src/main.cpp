#include "app/Application.hpp"
#include <exception>
#include <iostream>

// main entry point of the program
int main() {
  /*
   * this creates an obj named app of type rubiksim::Application
   * :: is scope resoltion - is used to identify and specify the context (scope)
   * to which an identifier (variable, function, or type) belongs. teh return
   * statement calls the run fucnt and returns its reult from main (1 or 0)
   */
  try {
    rubiksim::Application app;
    return app.run();
  }
  // means we recieve the excpetion by constant reference
  // a reference lets us access an existing object without copying it
  catch (const std::exception &error) {
    // is the standard error output stream
    // std::cout for normal output
    // std::cerr for error output
    // error.what() returns the error message
    // return 1 means the program failed
    std::cerr << "Fatal Error: " << error.what() << '\n';
    return 1;
  }
}

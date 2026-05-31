#include "app/Application.hpp"
// main entry point of the program
int main() {
  /*
   * this creates an obj named app of type rubiksim::Application
   * :: is scope resoltion - is used to identify and specify the context (scope)
   * to which an identifier (variable, function, or type) belongs. teh return
   * statement calls the run fucnt and returns its reult from main (1 or 0)
   */
  rubiksim::Application app;
  return app.run();
}

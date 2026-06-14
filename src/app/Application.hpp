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

namespace rubiksim {
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

  Application(const Application &) = delete;
  Application &operator=(const Application &) = delete;

  // This declares a member function
  /*
   * int run(); is not marked a const
   * coz rhe application loop will inberact with the window and application
   * state a const pormises not to modify the obj whidch is too strict for the
   * real app loop
   */
  int run();

  // means code outside tthe class cannot directly access what come after it
private:
  // * stores a pointer to the glfw window, a pointer stores a memory address
  // window_ is the variable name
  GLFWwindow *window_;
};
} // namespace rubiksim

// tells compiler to include this header once only per complimation unit --
// prevents duplicate class declarations from the same file
#pragma once
// creates named scope
namespace rubiksim {
// declares a class named Application
class Application {
  // everything after public is accessible from outsde the class
public:
  // This declares a member function
  // const means calling this function does not modify the object
  int run() const;
};
} // namespace rubiksim

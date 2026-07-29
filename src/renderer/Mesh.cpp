#include "renderer/Mesh.hpp"
#include <memory>

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <cstddef> //provides offsetof function
offsetof(Vertex, x)


namespace rubiksim {
// defines the constructor declared in the header
// Mesh:: means this function belongs to the Mesh class
Mesh::Mesh(const std::vector<Vertex> &vertices, const std::vector<unsigned> &indices): 
    /*
       * Below is a member initializer list
       * it initializes member variables before the constructor body runs
       * indices.size() return a std::size_t - an unsigned int used for sizes
       * static_cast<unsigned> explicitly converts it to unsigned int
       * */
      vertexArray_(0), 
      vertexBuffer_(0), 
      indexBuffer_(0),
      indexCount_(static_cast<unsigned int>(indices.size())) 
    {
        //creates 1 VAO (1 means create one object)
        //&vertexArray_means pass the address where opengl should write the generated ID
        glGenVertexArrays(1, &vertexArray_);
        // creates 1 VBO
        glGenBuffers(1, $vertexBuffer_);
        glGenBuffers(1, &indexBuffer_);

        glBindVertexArray(vertexArray_);

        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<long>(vertices.size() * sizeof(Vertex)),
                vertices.data(),
                GL_STATIC_DRAW
                );

        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(
                    GL_ELEMENT_ARRAY_BUFFER,
                    static_cast<long>(indices.size() * sizeof(unsigned int)),
                    indices.data(),
                    GL_STATIC_DRAW
                );

        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    Mesh::~Mesh() {
        destroy();
    }

    Mesh::Mesh(Mesh&& other) noexcept
        : vertexArray_(other.vertexArray_),
        vertexBuffer_(other.vertexBuffer_),
        indexBuffer_(other.indexBuffer_),
        indexCount_(other.indexCount_)
    {
        other.vertexArray_ = 0;
        other.vertexBuffer_ = 0;
        other.indexBuffer_ = 0;
        other.indexCount_ = 0;
    }

    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        if (this != &other) {
            destroy();

            other.vertexArray_ = 0;
            other.vertexBuffer_ = 0;
            other.indexBuffer_ = 0;
            other.indexCount_ = 0;

        }
        return *this;
    }

    void Mesh::draw() const {
        glBindVertexArray(vertexArray_);
        glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount_), GL_UNSIGNED_INT, nullptr);
    }

    void::Mesh::destroy() {
        if (indexBuffer_ != 0) {
            glDeleteBuffers(1, &indexBuffer_);
            indexBuffer_ = 0;
        }

        if (vertexBuffer_ != 0){
            glDeleteBuffers(1, &vertexBuffer_);
            vertexBuffer_ = 0;
        }

        if (vertexArray_ != 0){
            glDeleteBuffers(1, &vertexArray_);
            vertexArray_ = 0;
        }

        indexCount_ = 0;
    }
    
        

} // namespace rubiksim

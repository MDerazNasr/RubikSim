#include "renderer/Mesh.hpp"

#define GLFW_INCLUDE_GLCOREARB
#include <GLFW/glfw3.h>
#include <cstddef> //provides offsetof function

namespace rubiksim {
// defines the constructor declared in the header
// Mesh:: means this function belongs to the Mesh class
Mesh::Mesh(const std::vector<Vertex> &vertices,
           const std::vector<unsigned int> &indices):
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
        glGenBuffers(1, &vertexBuffer_);
        // creates one EBO - Element Buffer Object (stores indices)
        glGenBuffers(1, &indexBuffer_);
        // makes this VAO active 
        // After this opengl remmebers vertex attribute setup inside VAO
        glBindVertexArray(vertexArray_);

        // makes the VBO active as the current vertex buffer
        glBindBuffer(GL_ARRAY_BUFFER, vertexBuffer_);
        /*
         * copies vertex data from CPU memory into GPU memory 
         * vertices.size() is the number of vertex objeects 
         * sizeof(Vertex) is the numnber of bytes in one vertex
         * multiplying thenm gives the total byte count 
         * vertices.data() returns a raw pointer to the first elem in the vector
         * GL_STATIC_DRAW tells opengl we don not expect to change this data often
         *
         */
        glBufferData(
                GL_ARRAY_BUFFER,
                static_cast<long>(vertices.size() * sizeof(Vertex)),
                vertices.data(),
                GL_STATIC_DRAW
                );

        // makes the EBO active
        // Imp opengl detail - the EBO binding is stored inside the currently bound VAO, that is why we bind the VAO first
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indexBuffer_);
        glBufferData(
                    GL_ELEMENT_ARRAY_BUFFER,
                    static_cast<long>(indices.size() * sizeof(unsigned int)),
                    indices.data(),
                    GL_STATIC_DRAW
                );
        /*
         * This describes shader input location 0.
         * For us, location 0 means position.
         * 0 is the shader location.
         * 3 means three values.
         * GL_FLOAT means each value is a float.
         * GL_FALSE means do not normalize the values.
         * sizeof(Vertex) is the stride.
         * Stride means how many bytes OpenGL must jump to get from one vertex to the next.
         * reinterpret_cast<void*>
         * This converts the byte offset into the pointer type OpenGL expects.
         * This looks strange because OpenGL is a C API.
         * When a VBO is bound, the final argument is treated as a byte offset, not a real CPU pointer.
         */
        glVertexAttribPointer(0,3,GL_FLOAT,GL_FALSE,sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, x)));
        glEnableVertexAttribArray(0);
        /*
         * This describes shader input location 1.For us, location 1 means color.
         */
        glVertexAttribPointer(1,3,GL_FLOAT,GL_FALSE,sizeof(Vertex), reinterpret_cast<void*>(offsetof(Vertex, r)));
        
        glEnableVertexAttribArray(1);

        glBindVertexArray(0);
    }

    Mesh::~Mesh() {
        destroy();
    }

    /*
     * This creates a new Mesh by taking resources from another Mesh
     * After taking the IDs we set the old obj IDs to 0
     * That prevents the old obj from deleting resources it no longer owns
     */
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

    // this replaces an existing Mesh by taking resources from another one
    // Mesh& means it returns a reference to the current object 
    // operator= means we are defining assignment behaviour 
    // 
    Mesh& Mesh::operator=(Mesh&& other) noexcept {
        // this checks for self assignment
        // this is a pointer to the current object
        // &other is the address of the other object
        // if they are the same obj doing move assignment would be dangerous
        if (this != &other) {
            destroy();

            vertexArray_ = other.vertexArray_;
            vertexBuffer_ = other.vertexBuffer_;
            indexBuffer_ = other.indexBuffer_;
            indexCount_ = other.indexCount_;

            other.vertexArray_ = 0;
            other.vertexBuffer_ = 0;
            other.indexBuffer_ = 0;
            other.indexCount_ = 0;

        }
        return *this;
    }

    void Mesh::draw() const {
        glBindVertexArray(vertexArray_);
        /*
         * draws indexed geometry
         * GL_TRIANGLES means every 3 indices form one triangke 
         * static_cast<int>(indexCount_) given opengl the bymber of indices
         * GL_UNSIGNED_INT means each index is an unsigned integer
         * nullptr means start reading indicies at the start of the EBO
         */
        glDrawElements(GL_TRIANGLES, static_cast<int>(indexCount_), GL_UNSIGNED_INT, nullptr);
    }

    void Mesh::destroy() {
        if (indexBuffer_ != 0) {
            glDeleteBuffers(1, &indexBuffer_);
            indexBuffer_ = 0;
        }

        if (vertexBuffer_ != 0){
            glDeleteBuffers(1, &vertexBuffer_);
            vertexBuffer_ = 0;
        }

        if (vertexArray_ != 0){
            glDeleteVertexArrays(1, &vertexArray_);
            vertexArray_ = 0;
        }

        indexCount_ = 0;
    }
    
        

} // namespace rubiksim

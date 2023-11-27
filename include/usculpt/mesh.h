/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Mesh class
- the class allocates and initializes VBO, VAO, and EBO buffers, and it sets as OpenGL must consider the data in the buffers

VBO : Vertex Buffer Object - memory allocated on GPU memory to store the mesh data (vertices and their attributes, like e.g. normals, etc)
EBO : Element Buffer Object - a buffer maintaining the indices of vertices composing the mesh faces
VAO : Vertex Array Object - a buffer that helps to "manage" VBO and its inner structure. It stores pointers to the different vertex attributes stored in the VBO. When we need to render an object, we can just bind the corresponding VAO, and all the needed calls to set up the binding between vertex attributes and memory positions in the VBO are automatically configured.
See https://learnopengl.com/#!Getting-started/Hello-Triangle for details.

N.B. 1)
Model and Mesh classes follow RAII principles (https://en.cppreference.com/w/cpp/language/raii).
Mesh class is in charge of releasing the allocated GPU buffers, and
it is a "move-only" class. A move-only class ensures that you always have a 1:1 relationship between the total number of resources being created and the total number of actual instantiations occurring.
Moreover, we want to have, CPU-side, a Mesh instance, with associated GPU resources, which is responsible of their life cycle (RAII), and which could be "moved" in memory keeping the ownership of its resources

N.B. 2) no texturing in this version of the class

N.B. 3) based on https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/mesh.h

author: Andrea Cipollini; based on RTGP course code by prof. Davide Gadia and by Michael Marchesan
*/

#pragma once

using namespace std;

// Std. Includes
#include <vector>

// data structure for vertices
struct Vertex {
    // vertex coordinates
    glm::vec3 Position;
    // Normal
    glm::vec3 Normal;
    // Texture coordinates
    glm::vec2 TexCoords;
    // Tangent
    glm::vec3 Tangent;
    // Bitangent
    glm::vec3 Bitangent;

    // neighbours data
    GLuint NeighboursIndex;
    GLuint NeighboursNumber;
};

// different types of rendering
enum RenderingType { TRIANGLES, LINES };

// the intersection struct stores the intersection point in world coordinates and the index of the hitted primitive of the mesh
// if the primitive index is equal to -1 -> there is not intersection
struct Intersection
{
    glm::vec3 Position;
    glm::vec3 Normal;
    bool hit;
    GLuint idxv0, idxv1, idxv2;
};

/////////////////// MESH class ///////////////////////
class Mesh {
public:
    // data structures for vertices, and indices of vertices (for faces)
    vector<Vertex> vertices;
    vector<GLuint> indices;
    vector<GLuint> neighbours;
    // VAO
    GLuint VAO;

    // We want Mesh to be a move-only class. We delete copy constructor and copy assignment
    // see:
    // https://docs.microsoft.com/en-us/cpp/cpp/constructors-cpp?view=vs-2019
    // https://en.cppreference.com/w/cpp/language/copy_constructor
    // https://en.cppreference.com/w/cpp/language/copy_assignment
    // https://www.geeksforgeeks.org/preventing-object-copy-in-cpp-3-different-ways/
    // Section 4.6 of the "A Tour in C++" book
    Mesh(const Mesh& copy) = delete; //disallow copy
    Mesh& operator=(const Mesh &) = delete;

    // Constructor
    // We use initializer list and std::move in order to avoid a copy of the arguments
    // This constructor empties the source vectors (vertices and indices)
    Mesh(vector<Vertex>& vertices, vector<GLuint>& indices) noexcept
        : vertices(std::move(vertices)), indices(std::move(indices))
    {
        this->setupMesh();
    }

    // Constructor
    Mesh(vector<Vertex>& vertices, vector<GLuint>& indices, vector<GLuint>& neighbours) noexcept
        : vertices(std::move(vertices)), indices(std::move(indices)), neighbours(std::move(neighbours))
    {
        this->setupMesh();
    }

    // We implement a user-defined move constructor and move assignment
    // see:
    // https://docs.microsoft.com/en-us/cpp/cpp/move-constructors-and-move-assignment-operators-cpp?view=vs-2019
    // https://en.cppreference.com/w/cpp/language/move_constructor
    // https://en.cppreference.com/w/cpp/language/move_assignment
    // https://www.learncpp.com/cpp-tutorial/15-1-intro-to-smart-pointers-move-semantics/
    // https://www.learncpp.com/cpp-tutorial/15-3-move-constructors-and-move-assignment/
    // Section 4.6 of the "A Tour in C++" book

    // Move constructor
    // The source object of a move constructor is not expected to be valid after the move.
    // In our case it will no longer imply ownership of the GPU resources and its vectors will be empty.
    Mesh(Mesh&& move) noexcept
        // Calls move for both vectors, which internally consists of a simple pointer swap between the new instance and the source one.
        : vertices(std::move(move.vertices)), indices(std::move(move.indices)), neighbours(std::move(move.neighbours)),
        VAO(move.VAO), VBO(move.VBO), EBO(move.EBO)
    {
        move.VAO = 0; // We *could* set VBO and EBO to 0 too,
        // but since we bring all the 3 values around we can use just one of them to check ownership of the 3 resources.
    }

    // Move assignment
    Mesh& operator=(Mesh&& move) noexcept
    {
        // calls the function which will delete (if needed) the GPU resources for this instance
        freeGPUresources();

        if (move.VAO) // source instance has GPU resources
        {
            vertices = std::move(move.vertices);
            indices = std::move(move.indices);
            neighbours = std::move(move.neighbours);
            VAO = move.VAO;
            VBO = move.VBO;
            EBO = move.EBO;

            move.VAO = 0;
        }
        else // source instance was already invalid
        {
            VAO = 0;
        }
        return *this;
    }

    // destructor
    ~Mesh() noexcept
    {
        // calls the function which will delete (if needed) the GPU resources
        freeGPUresources();
    }

    //////////////////////////////////////////

    // rendering of mesh
    void Draw(GLuint buffer, RenderingType renderingType = TRIANGLES)
    {
        // VAO is made "active"
        glBindVertexArray(buffer);
        // rendering of data in the VAO
        if (renderingType == TRIANGLES)
            glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
        else if (renderingType == LINES)
            glDrawElements(GL_LINES, this->indices.size(), GL_UNSIGNED_INT, 0);
        // VAO is "detached"
        glBindVertexArray(0);
    }

    void Draw(RenderingType renderingType = TRIANGLES)
    {
        // VAO is made "active"
        glBindVertexArray(this->VAO);
        // rendering of data in the VAO
        if (renderingType == TRIANGLES)
            glDrawElements(GL_TRIANGLES, this->indices.size(), GL_UNSIGNED_INT, 0);
        else if (renderingType == LINES)
            glDrawElements(GL_LINES, this->indices.size(), GL_UNSIGNED_INT, 0);
        // VAO is "detached"
        glBindVertexArray(0);
    }

    // bind mesh data on GPU shader buffer
    void InitMeshUpdate()
    {
        // vertices
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->VAO);
        // indices
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, this->EBO);
        // neighbours
        glGenBuffers(1, &this->NeighboursBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, this->NeighboursBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(GLuint) * this->neighbours.size(), &this->neighbours[0], GL_DYNAMIC_DRAW);

        ResetIntersectionData();

        //glBufferData(GL_SHADER_STORAGE_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_DYNAMIC_DRAW);

        /*
        vector<glm::vec3> positions;
        for (int i = 0; i < this->vertices.size(); i++)
        {
            positions.push_back(this->vertices[i].Position);
        }

        GLuint temp;
        glGenBuffers(1, &temp);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, temp);
        glBufferData(GL_SHADER_STORAGE_BUFFER, positions.size() * sizeof(glm::vec3), &positions[0], GL_DYNAMIC_DRAW);
        */
    }

    void ResetIntersectionData()
    {
        // create buffer object for intersection data
        Intersection inter = {glm::vec3(0.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 0.0f), false, (GLuint)-1, (GLuint)-1, (GLuint)-1};
        glGenBuffers(1, &this->IntersectionBuffer);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, this->IntersectionBuffer);
        glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Intersection), &inter, GL_DYNAMIC_DRAW);
    }

    void UpdateNormals()
    {
        for (int i = 0; i < this->vertices.size(); i++)
        {
            glm::vec3 newNormal = glm::vec3(0.0f, 0.0f, 0.0f);
            for (int j = this->vertices[i].NeighboursIndex; j < this->vertices[i].NeighboursIndex + this->vertices[i].NeighboursNumber - 1; j += 2)
            {
                int k = this->neighbours[j];
                int h = this->neighbours[j + 1];
                glm::vec3 e1 = glm::vec3(this->vertices[k].Position.x, this->vertices[k].Position.y, this->vertices[k].Position.z) - this->vertices[i].Position;
                glm::vec3 e2 = glm::vec3(this->vertices[h].Position.x, this->vertices[h].Position.y, this->vertices[h].Position.z) - this->vertices[i].Position;

                glm::vec3 faceNormal = glm::cross(e1, e2);
                float angleDot = glm::dot(e1, e2);
                if (glm::dot(faceNormal, this->vertices[i].Normal) < .0f)
                    faceNormal = -faceNormal;

                newNormal += faceNormal * glm::acos((e1 * e2) / (glm::length(e1) * glm::length(e2)));
            }
            /*
            int k = this->neighbours[this->vertices[i].NeighboursIndex + this->vertices[i].NeighboursNumber - 1];
            int h = this->neighbours[this->vertices[i].NeighboursIndex];
            glm::vec3 e1 = glm::vec3(this->vertices[k].Position.x, this->vertices[k].Position.y, this->vertices[k].Position.z) - this->vertices[i].Position;
            glm::vec3 e2 = glm::vec3(this->vertices[h].Position.x, this->vertices[h].Position.y, this->vertices[h].Position.z) - this->vertices[i].Position;

            glm::vec3 faceNormal = glm::cross(e1, e2);
            float angleDot = glm::dot(e1, e2);
            if (glm::dot(faceNormal, this->vertices[i].Normal) < .0f)
                faceNormal = -faceNormal;

            newNormal += faceNormal; //* glm::acos((e1 * e2) / (glm::length(e1) * glm::length(e2)));
            */
            // check orientation before apply
            float check = glm::dot(this->vertices[i].Normal, newNormal);
            if (check < .0f)
                newNormal = -newNormal;

            this->vertices[i].Normal = glm::normalize(newNormal);
        }
    }

private:

    // VBO and EBO
    GLuint VBO, EBO, IntersectionBuffer, NeighboursBuffer;

    //////////////////////////////////////////
    // buffer objects\arrays are initialized
    // a brief description of their role and how they are binded can be found at:
    // https://learnopengl.com/#!Getting-started/Hello-Triangle
    // (in different parts of the page), or here:
    // http://www.informit.com/articles/article.aspx?p=1377833&seqNum=8
    void setupMesh()
    {
        UpdateNormals();

        // we create the buffers
        glGenVertexArrays(1, &this->VAO);
        glGenBuffers(1, &this->VBO);
        glGenBuffers(1, &this->EBO);

        // VAO is made "active"
        glBindVertexArray(this->VAO);
        // we copy data in the VBO - we must set the data dimension, and the pointer to the structure cointaining the data
        glBindBuffer(GL_ARRAY_BUFFER, this->VBO);
        glBufferData(GL_ARRAY_BUFFER, this->vertices.size() * sizeof(Vertex), &this->vertices[0], GL_DYNAMIC_DRAW);
        // we copy data in the EBO - we must set the data dimension, and the pointer to the structure cointaining the data
        glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, this->EBO);
        glBufferData(GL_ELEMENT_ARRAY_BUFFER, this->indices.size() * sizeof(GLuint), &this->indices[0], GL_DYNAMIC_DRAW);

        // we set in the VAO the pointers to the different vertex attributes (with the relative offsets inside the data structure)
        // vertex positions
        // these will be the positions to use in the layout qualifiers in the shaders ("layout (location = ...)"")
        glEnableVertexAttribArray(0);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)0);
        // Normals
        glEnableVertexAttribArray(1);
        glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Normal));
        // Texture Coordinates
        glEnableVertexAttribArray(2);
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, TexCoords));
        // Tangent
        glEnableVertexAttribArray(3);
        glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Tangent));
        // Bitangent
        glEnableVertexAttribArray(4);
        glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, Bitangent));
        // Neighbours
        glEnableVertexAttribArray(5);
        glVertexAttribPointer(5, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, NeighboursIndex));
        glEnableVertexAttribArray(6);
        glVertexAttribPointer(6, 1, GL_FLOAT, GL_FALSE, sizeof(Vertex), (GLvoid*)offsetof(Vertex, NeighboursNumber));

        glBindVertexArray(0);
    }

    //////////////////////////////////////////

    void freeGPUresources()
    {
        // If VAO is 0, this instance of Mesh has been through a move, and no longer owns GPU resources,
        // so there's no need for deleting.
        if (VAO)
        {
            glDeleteVertexArrays(1, &this->VAO);
            glDeleteBuffers(1, &this->VBO);
            glDeleteBuffers(1, &this->EBO);
        }
    }
};
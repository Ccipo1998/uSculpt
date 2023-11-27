/*
Texture class:
- 
*/

#pragma once

using namespace  std;

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>
#include <glm.hpp>
#include <glad.h>
#include <stb_image/stb_image.h>

////////// TEXTURE class //////////
class Texture
{
public:
    GLuint width;
    GLuint height;
    const unsigned char* content;

    // constructor
    Texture(const char* path) {
        this->loadTexture(path);
    }

    // destructor
    ~Texture() noexcept
    {
        // calls the function which will delete (if needed) the GPU resources
        freeGPUresources();
    }

private:
    // id of the object in OpenGL
    GLuint tex;

    void loadTexture(const char* path) {
        int width, height;
        // we load the image
        unsigned char* data = stbi_load(path, &width, &height, nullptr, 0);
        
        if (data != nullptr) 
        {
            this->width = width;
            this->height = height;
            this->content = data;

            this->setupTexture();
        }
        else 
        {
            cout << "FAILED TO LOAD TEXTURE" << endl;
        }

        stbi_image_free(data);
    }

    void setupTexture() {
        // it assigns the new id into tex variable
        glGenTextures(1, &this->tex);
        // set new object to the target GL_TEXTURE_2D
        glBindTexture(GL_TEXTURE_2D, this->tex);
        // allocation of immutable storage for the texture
        glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA8, this->width, this->height);
        // copy the data in the texture object
        glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, this->width, this->height, GL_RGB, GL_UNSIGNED_BYTE, this->content);

        // setting for the rendering of the texture (see the lectures)
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_BORDER);	
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_BORDER);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

        // set the texture to the texture unit 0
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, this->tex);
    }

    void freeGPUresources() {
        glDeleteTextures(1, &this->tex);
    }
};




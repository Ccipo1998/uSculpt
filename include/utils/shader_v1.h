/*
Shader class - v1
- loading Shader source code, Shader Program creation

N.B. ) adaptation of https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/shader.h

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano
*/

#pragma once

using namespace std;

// Std. Includes
#include <string>
#include <fstream>
#include <sstream>
#include <iostream>

/////////////////// SHADER class ///////////////////////
class Shader
{
public:
    GLuint Program;

    //////////////////////////////////////////

    //constructor
    Shader(const GLchar* vertexPath, const GLchar* fragmentPath)
    {
        // Step 1: we retrieve shaders source code from provided filepaths
        string vertexCode;
        string fragmentCode;
        ifstream vShaderFile;
        ifstream fShaderFile;

        // ensure ifstream objects can throw exceptions:
        vShaderFile.exceptions (ifstream::failbit | ifstream::badbit);
        fShaderFile.exceptions (ifstream::failbit | ifstream::badbit);
        try
        {
            // Open files
            vShaderFile.open(vertexPath);
            fShaderFile.open(fragmentPath);
            stringstream vShaderStream, fShaderStream;
            // Read file's buffer contents into streams
            vShaderStream << vShaderFile.rdbuf();
            fShaderStream << fShaderFile.rdbuf();
            // close file handlers
            vShaderFile.close();
            fShaderFile.close();
            // Convert stream into string
            vertexCode = vShaderStream.str();
            fragmentCode = fShaderStream.str();
        }
        catch (ifstream::failure e)
        {
            cout << "ERROR::SHADER::FILE_NOT_SUCCESFULLY_READ" << endl;
        }

        // Convert strings to char pointers
        const GLchar* vShaderCode = vertexCode.c_str();
        const GLchar * fShaderCode = fragmentCode.c_str();

        // Step 2: we compile the shaders
        GLuint vertex, fragment;

        // Vertex Shader
        vertex = glCreateShader(GL_VERTEX_SHADER);
        glShaderSource(vertex, 1, &vShaderCode, NULL);
        glCompileShader(vertex);
        // check compilation errors
        checkCompileErrors(vertex, "VERTEX");

        // Fragment Shader
        fragment = glCreateShader(GL_FRAGMENT_SHADER);
        glShaderSource(fragment, 1, &fShaderCode, NULL);
        glCompileShader(fragment);
        // check compilation errors
        checkCompileErrors(fragment, "FRAGMENT");

        // Step 3: Shader Program creation
        this->Program = glCreateProgram();
        glAttachShader(this->Program, vertex);
        glAttachShader(this->Program, fragment);
        glLinkProgram(this->Program);
        // check linking errors
        checkCompileErrors(this->Program, "PROGRAM");

        // Step 4: we delete the shaders because they are linked to the Shader Program, and we do not need them anymore
        glDeleteShader(vertex);
        glDeleteShader(fragment);
    }

    //////////////////////////////////////////

    // We activate the Shader Program as part of the current rendering process
    void Use() { glUseProgram(this->Program); }

    // We delete the Shader Program when application closes
    void Delete() { glDeleteProgram(this->Program); }

private:
    //////////////////////////////////////////

    // Check compilation and linking errors
    void checkCompileErrors(GLuint shader, string type)
	{
		GLint success;
		GLchar infoLog[1024];
		if(type != "PROGRAM")
		{
			glGetShaderiv(shader, GL_COMPILE_STATUS, &success);
			if(!success)
			{
				glGetShaderInfoLog(shader, 1024, NULL, infoLog);
                cout << "| ERROR::::SHADER-COMPILATION-ERROR of type: " << type << "|\n" << infoLog << "\n| -- --------------------------------------------------- -- |" << endl;
			}
		}
		else
		{
			glGetProgramiv(shader, GL_LINK_STATUS, &success);
			if(!success)
			{
				glGetProgramInfoLog(shader, 1024, NULL, infoLog);
                cout << "| ERROR::::PROGRAM-LINKING-ERROR of type: " << type << "|\n" << infoLog << "\n| -- --------------------------------------------------- -- |" << endl;
			}
		}
	}
};

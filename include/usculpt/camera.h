/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Camera class
- creation of a reference system for the camera
- management of camera movement (limited FPS-style) using WASD keys

N.B.) adaptation of https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/camera.h

author: Andrea Cipollini; based on RTGP course code by prof. Davide Gadia
*/

#pragma once

// we use GLM to create the view matrix and to manage camera transformations
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

// possible camera movements
enum Camera_Movement {
    FORWARD,
    BACKWARD,
    LEFT,
    RIGHT
};

// Default camera settings
// Initial camera orientation on Y and X (Z not considered)
const GLfloat YAW        = -90.0f; //Y
const GLfloat PITCH      =  0.0f; //X

// parameters to manage mouse movement
const GLfloat SPEED      =  3.0f;
const GLfloat SENSITIVITY =  0.25f;

// Ray struct
struct Ray3
{
    glm::vec3 origin;
    glm::vec3 direction;
};

///////////////////  CAMERA class ///////////////////////
class Camera
{
public:
    // Camera Attributes
    glm::vec3 Position;
    glm::vec3 Front;
    glm::vec3 WorldFront;
    glm::vec3 Up; // camera local UP vector
    glm::vec3 Right;
    glm::vec3 WorldUp; //  camera world UP vector -> needed for the initial computation of Right vector
    GLboolean onGround; // it defines if the camera is "anchored" to the ground, or if it strictly follows the current Front direction (even if this means that the camera "flies" in the scene)
    // N.B.) this version works only for flat terrains

    // Eular Angles
    GLfloat Yaw;
    GLfloat Pitch;

    // Camera options
    GLfloat MovementSpeed;
    GLfloat MouseSensitivity;

    // Parameters for Projection Matrix
    GLfloat Fov;
    GLuint ScreenWidth;
    GLuint ScreenHeight;
    GLfloat NearPlane;
    GLfloat FarPlane;

    // current Camera Ray
    Ray3 CameraRay;

    //////////////////////////////////////////
    // simplified constructor
    // it can be extended passing different values of speed and mouse sensitivity, etc...
    Camera(glm::vec3 position, GLboolean onGround, GLfloat fov, GLuint screenWidth, GLuint screenHeight, GLfloat near, GLfloat far)
        :Position(position),onGround(onGround),Yaw(YAW),Pitch(PITCH),MovementSpeed(SPEED),MouseSensitivity(SENSITIVITY),Fov(fov),ScreenWidth(screenWidth),ScreenHeight(screenHeight),NearPlane(near),FarPlane(far)
    {
        this->WorldUp = glm::vec3(0.0f,1.0f,0.0f);
        // initialization of the camera reference system
        this->updateCameraVectors();
        // camera ray is initialized with the position of the camera and front direction
        this->CameraRay = Ray3 { this->Position, this->Front };
    }

    //////////////////////////////////////////
    // it returns the current view matrix
    glm::mat4 GetViewMatrix()
    {
        return glm::lookAt(this->Position, this->Position + this->Front, this->Up);
    }

    //////////////////////////////////////////
    // it returns the current projection matrix
    glm::mat4 GetProjectionMatrix()
    {
        return glm::perspective(this->Fov, (float)this->ScreenWidth / (float)this->ScreenHeight, this->NearPlane, this->FarPlane);
    }

    //////////////////////////////////////////
    // it updates camera position when a WASD key is pressed
    void ProcessKeyboard(Camera_Movement direction, GLfloat deltaTime)
    {
        GLfloat velocity = this->MovementSpeed * deltaTime;
        if (direction == FORWARD)
            this->Position += (this->onGround ? this->WorldFront : this->Front) * velocity;
        if (direction == BACKWARD)
            this->Position -= (this->onGround ? this->WorldFront : this->Front) * velocity;
        if (direction == LEFT)
            this->Position -= this->Right * velocity;
        if (direction == RIGHT)
            this->Position += this->Right * velocity;
    }

    //////////////////////////////////////////
    // it updates camera orientation when mouse is moved
    void ProcessMouseMovement(GLfloat xoffset, GLfloat yoffset, GLboolean constraintPitch = GL_TRUE)
    {
        // the sensitivity is applied to weight the movement
        xoffset *= this->MouseSensitivity;
        yoffset *= this->MouseSensitivity;

        // rotation angles on Y and X are updated
        this->Yaw   += xoffset;
        this->Pitch += yoffset;

        // we apply a constraint to the rotation on X, to avoid to have the camera flipped upside down
        // N.B.) this constraint helps to avoid gimbal lock, if all the 3 rotations are considered
        if (constraintPitch)
        {
            if (this->Pitch > 89.0f)
                this->Pitch = 89.0f;
            if (this->Pitch < -89.0f)
                this->Pitch = -89.0f;
        }

        // the camera reference system is updated using the new camera rotations
        this->updateCameraVectors();
    }

    //////////////////////////////////////////
    // it updates the camera ray according to current mouse cursor position
    void UpdateCameraRay(GLfloat mouseX, GLfloat mouseY)
    {
        // we need the inverse matrix to pass from screen space to world space
        glm::mat4 projection = this->GetProjectionMatrix();
        glm::mat4 view = this->GetViewMatrix();

        glm::mat4 inverse = glm::inverse(projection * view);

        // from screen space of mouse X and Y to normalized device space in [-1, 1] on u and v
        float u = mouseX / (float)this->ScreenWidth;
        float v = mouseY / (float)this->ScreenHeight;
        u = u * 2.0f - 1.0f;
        v = 1.0f - v * 2;

        // from normalized device space to homogeneus clip space -> eye space -> world space
        glm::vec4 in = glm::vec4(u, v, 1.0f, 1.0f);
        glm::vec4 out = inverse * in;
        // perspective division
        out.w = 1.0f / out.w;
        out.x *= out.w;
        out.y *= out.w;
        out.z *= out.w;

        // the origin of the ray is the camera position (the ray comes out from the camera)
        this->CameraRay.origin = this->Position;
        // the direction of the ray is the vector from the origin to the mouse cursor position in world coordinates (its position on the viewport in world coordinates)
        this->CameraRay.direction = glm::normalize(glm::vec3(out.x, out.y, out.z) - this->CameraRay.origin);
    }

private:
    //////////////////////////////////////////
    // it updates the camera reference system
    void updateCameraVectors()
    {
        // it computes the new Front vector using trigonometric calculations using Yaw an Pitch angles
        // https://learnopengl.com/#!Getting-started/Camera
        glm::vec3 front;
        front.x = cos(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        front.y = sin(glm::radians(this->Pitch));
        front.z = sin(glm::radians(this->Yaw)) * cos(glm::radians(this->Pitch));
        this->WorldFront = this->Front = glm::normalize(front);
        // if the camera is "anchored" to the ground, the world Front vector is equal to the local Front vector, but with the y component = 0
        this->WorldFront.y = 0.0f;
        // Once calculated the new view direction, we re-calculate the Right vector as cross product between Front and world Up vector
        this->Right = glm::normalize(glm::cross(this->Front, this->WorldUp));
        // we calculate the camera local Up vector as cross product between Front and Right vectors
        this->Up    = glm::normalize(glm::cross(this->Right, this->Front));
    }
};

/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Real-Time Graphics Programming degree course project:
        tiny GPU-based (OPENGL, GLSL) real-time digital sculpting tool

author: Andrea Cipollini -> based on RTGP course code by prof. Davide Gadia
                            based on ysculpting (https://github.com/Ccipo1998/Bachelor-Degree-Internship) by Andrea Cipollini and prof. Fabio Pellacini

Main Class
*/

#pragma region INCLUDES

// Std. Includes
#include <string>
#include <stdlib.h>
#include <iomanip>

// Loader estensioni OpenGL
// http://glad.dav1d.de/
// THIS IS OPTIONAL AND NOT REQUIRED, ONLY USE THIS IF YOU DON'T WANT GLAD TO INCLUDE windows.h
// GLAD will include windows.h for APIENTRY if it was not previously defined.
// Make sure you have the correct definition for APIENTRY for platforms which define _WIN32 but don't use __stdcall
#ifdef _WIN32
    #define APIENTRY __stdcall
#endif

// glad is a loader generator for the setup of OpenGL context -> library for all OpenGL functionalities which are not provided by Microsoft libraries anymore
#include <glad/glad.h>

// GLFW library to create window and to manage I/O
#include <glfw/glfw3.h>

// another check related to OpenGL loader
// confirm that GLAD didn't include windows.h
#ifdef _WINDOWS_
    #error windows.h was included!
#endif

// classes developed during lab lectures to manage shaders, to load models, for FPS camera, and for physical simulation
#include <utils/shader.h>
#include <utils/model.h>
#include <utils/camera.h>
#include <utils/texture.h>

// glm is a robust library to manage matrix and vector operations (with matrix and vector classes ready-to-use) -> use glm namespace!
// it is not recommended to include all the glm headers with "using namespace glm" -> it can lead to many namespace clashes
// glm support passing a glm-type to OpenGL with glm::value_ptr(glm object)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/matrix_inverse.hpp>
#include <glm/gtc/type_ptr.hpp>

// we include the library for images loading
#define STB_IMAGE_IMPLEMENTATION
#include <stb_image/stb_image.h>

// include for gui
#include <imgui/imgui.h>
#include <imgui/imgui_impl_opengl3.h>
#include <imgui/imgui_impl_glfw.h>

#pragma endregion INCLUDES

// windows' dimensions
GLuint screenWidth = 1500, screenHeight = 900;

// boolean to activate/deactivate wireframe rendering
GLboolean wireframe = GL_FALSE;

#pragma region FUNCTION DECLARATIONS

// callback functions for keyboard and mouse events (events handle for user commands)
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode);
void mouse_callback(GLFWwindow* window, double xpos, double ypos);
void mouse_key_callback(GLFWwindow* window, int button, int action, int mods);
// if one of the WASD keys is pressed, we call the corresponding method of the Camera class
void apply_camera_movements();

#pragma endregion FUNCTION DECLARATIONS

#pragma region EVENT PARAMETERS

// we initialize an array of booleans for each keybord key
bool keys[1024];

// we set the initial position of mouse cursor in the application window
GLfloat lastX = 0.0f, lastY = 0.0f;

// when rendering the first frame, we do not have a "previous state" for the mouse, so we need to manage this situation
bool firstMouse = true;

// mouse callbacks flags
bool brush = false;
bool rotation = false;

#pragma endregion EVENT PARAMETERS

#pragma region FRAME PARAMETERS

// parameters for movements and fps counter
GLfloat deltaTime = 0.0f;
GLfloat lastFrame = 0.0f;
GLfloat lastFPS = 0.0f;
GLfloat deltaFPS = 0.0f;
unsigned int fps = 0;

#pragma endregion FRAME PARAMETERS

#pragma region PROJECTION

// view and projection matrices (global because we need to use them in the keyboard callback)
glm::mat4 view, projection;

// we create a camera. We pass the initial position as a parameter to the constructor. In this case, we use a "floating" camera (we pass false as second parameter)
Camera camera(glm::vec3(0.0f, 0.0f, 1.5f), GL_FALSE, 45.0f, screenWidth, screenHeight, 0.1f, 1000.0f); // the model is in position by default (0, 0, 0)

#pragma endregion PROJECTION

#pragma region SHADER UNIFORMS

// Uniforms to be passed to shaders

// point light position (camera eye light)
glm::vec3 pointLightPosition = camera.Position - glm::vec3(0.0f, 0.0f, 0.05f); // little bit behind the camera (to avoid artifacts)
// illumination model parameters
// weight for the diffusive component
GLfloat Kd = 3.0f;
// roughness index for GGX shader
GLfloat alpha = 1.0f;
// Fresnel reflectance at 0 degree (Schlik's approximation)
GLfloat F0 = 0.1f;

#pragma endregion SHADER UNIFORMS

#pragma region MODEL PARAMETERS

// diffuse color of the model
GLfloat diffuseColor[] = {0.90f, 0.55f, 0.39f};
// ambient color on the model
GLfloat ambientColor[] = {0.15f, 0.15f, 0.15f};
// specular color on the model
GLfloat specularColor[] = {1.0f, 1.0f, 1.0f};

// model default parameters
glm::vec3 model_pos = glm::vec3(0.0f, 0.0f, 0.0f);
glm::vec3 model_scale = glm::vec3(1.0f, 1.0f, 1.0f);
glm::mat4 modelMatrix = glm::mat4(1.0f);

#pragma endregion MODEL PARAMETERS

#pragma region SCULPTING PARAMETERS

// sculpting params
float radius = 0.25f;
float strength = 1.0f;

#pragma endregion SCULPTING PARAMETERS

////////////////// MAIN function ///////////////////////
// until the game loop, here we enter the application stage
int main()
{
    #pragma region WINDOW AND CONTEXT INIT

    // Initialization of OpenGL context using GLFW
    glfwInit();
    // We set OpenGL specifications required for this application
    // In this case: 4.1 Core (me: i modified the core version to follow the cookbook specs)
    // If not supported by your graphics HW, the context will not be created and the application will close
    // N.B.) creating GLAD code to load extensions, try to take into account the specifications and any extensions you want to use,
    // in relation also to the values indicated in these GLFW commands
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    // we set if the window is resizable
    glfwWindowHint(GLFW_RESIZABLE, GL_FALSE);

    // we create the application's window that we can use for GLFW's functions
    GLFWwindow* window = glfwCreateWindow(screenWidth, screenHeight, "uSculpt", nullptr, nullptr);
    if (!window)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);

    // we put in relation the window and the callbacks to handle events of user commands
    glfwSetKeyCallback(window, key_callback);
    glfwSetCursorPosCallback(window, mouse_callback);
    glfwSetMouseButtonCallback(window, mouse_key_callback);

    // we could disable the mouse cursor
    //glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);

    // GLAD tries to load the context set by GLFW
    if (!gladLoadGLLoader((GLADloadproc) glfwGetProcAddress))
    {
        std::cout << "Failed to initialize OpenGL context" << std::endl;
        return -1;
    }

    // After the init of the context, we can query the current supported version on OpenGL and GLSL
    const GLubyte *renderer = glGetString(GL_RENDERER);
    const GLubyte *vendor = glGetString(GL_VENDOR);
    const GLubyte *version = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);
    GLint major, minor;
    glGetIntegerv(GL_MAJOR_VERSION, &major);
    glGetIntegerv(GL_MINOR_VERSION, &minor);
    printf("GL Vendor               :%s\n", vendor);
    printf("GL Renderer             :%s\n", renderer);
    printf("GL Version (string)     :%s\n", version);
    printf("GL Version (integer)    :%d.%d\n", major, minor);
    printf("GLSL version            :%s\n", glslversion);

    #pragma endregion WINDOW AND CONTEXT INIT

    // we define the viewport dimensions and position compared to the window
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);

    // we enable Z test
    glEnable(GL_DEPTH_TEST);

    //the "clear" color for the frame buffer
    glClearColor(0.15f, 0.15f, 0.15f, 1.0f);

    #pragma region TEXTURE TODO

    // TODO: utilizzare texture per fare il mirino
    // After loading the default model, we need to set up the texture representing the range of action of the brush on the model
    /*
    TEXTURES:
    OpenGL 4.2 introduced "immutable storage textures" -> immutable refers to the storage itself, like size, format ecc... which are fixed; the content can change
                                                       -> immutable storage is useful in majority of cases, cause it make possible to avoid consistency checks, leading to type safety
                                                       -> those are allocated using glTexStorage*
    In GLSL texture are accessed via "sampler" variables -> handle to a texture unit
                                                         -> declared as uniform variables of shaders
                                                         -> initialized within the application stage to point the appropriate texture
    Texture BrushSight("textures/sight.png");
    int loc = glGetUniformLocation(shader.Program, "SightTex");
    glUniform1i(loc, 0);
    */

    #pragma endregion TEXTURE TODO

    #pragma region MODEL INIT

    // loading of an initial standard sphere mesh
    Model model("models/sphere1000k.obj");

    // Model and Normal transformation matrices for the model
    modelMatrix = glm::translate(glm::mat4(1.0f), model_pos);
    modelMatrix = glm::scale(modelMatrix, model_scale);
    glm::mat4 normalmatrix = glm::mat4(1.0f);

    #pragma endregion MODEL INIT

    // the choose Shader Program for the objects used in the application
    Shader shader = Shader("ShaderBrushing.vert", "ShaderRendering.frag", "ShaderIntersection.geom");

    // Projection matrix: FOV angle, aspect ratio, near and far planes (all setted in camera class to retrieve the matrix if needed)
    projection = camera.GetProjectionMatrix();
    // camera-ray functions for intersection (init)
    camera.UpdateCameraRay(0, 0);

    // unit cube visualization for debugging
    //Model UnitCube("models/cube.obj");

    #pragma region TRANSFORM FEEDBACK INIT

    // buffer objects for transform feedback and SSBO
    GLuint VAOs[2], feedback[2], vertices[2], inters[2];
    model.meshes[0].InitMeshUpdate(VAOs, feedback, vertices, inters);

    // switch between input and output for transform feedback and rendering
    int drawBuf = 1;

    #pragma endregion TRANSFORM FEEDBACK INIT

    #pragma region GUI INIT

    // gui init
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 460");

    #pragma endregion GUI INIT

    // Rendering loop: this code is executed at each frame
    while(!glfwWindowShouldClose(window))
    {
        #pragma region FRAME TIME

        // we determine the time passed from the beginning
        // and we calculate time difference between current frame rendering and the previous one
        GLfloat currentFrame = glfwGetTime();
        deltaTime = currentFrame - lastFrame;
        lastFrame = currentFrame;

        // update fps counter
        deltaFPS = currentFrame - lastFPS;
        fps++;
        if (deltaFPS >= 1.0f)
        {
            cout << '\r' << std::setw(2) << "FPS: " << (unsigned int)((1.0f / deltaFPS) * fps) << std::flush;
            lastFPS = currentFrame;
            fps = 0;
        }

        #pragma endregion FRAME TIME

        #pragma region GUI RENDERING

        // gui
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        ImGui::Begin("Sculpting parameters"); 
        ImGui::SliderFloat("Radius", &radius, 0.01f, 0.5f);
        ImGui::SliderFloat("Strength", &strength, 0.1f, 3.0f);
        ImGui::End();
        ImGui::Render();

        #pragma endregion GUI RENDERING

        // Check if an I/O event is happening
        glfwPollEvents();

        // we apply camera movements
        apply_camera_movements();

        // View matrix (=camera): position, view direction, camera "up" vector
        view = camera.GetViewMatrix();

        // we "clear" the frame and z buffer
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // we set the rendering mode
        if (wireframe)
            // Draw in wireframe
            glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
        else
            glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

        // Mouse ray update for intersection test
        camera.UpdateCameraRay(lastX, lastY);

        // select the shader to use
        shader.Use();

        // We search inside the Shader Program the name of a subroutine, and we get the numerical index
        GLuint index = glGetSubroutineIndex(shader.Program, GL_FRAGMENT_SHADER, "GGX");
        // we activate the subroutine using the index
        glUniformSubroutinesuiv( GL_FRAGMENT_SHADER, 1, &index);

        #pragma region UNIFORMS

        // projection and view matrix for intersection and rendering
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "projectionMatrix"), 1, GL_FALSE, glm::value_ptr(projection));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "viewMatrix"), 1, GL_FALSE, glm::value_ptr(view));

        // colors, light and illumination
        glUniform3fv(glGetUniformLocation(shader.Program, "diffuseColor"), 1, diffuseColor);
        glUniform3fv(glGetUniformLocation(shader.Program, "pointLightPosition"), 1, glm::value_ptr(pointLightPosition));
        glUniform1f(glGetUniformLocation(shader.Program, "Kd"), Kd);
        glUniform1f(glGetUniformLocation(shader.Program, "alpha"), alpha);
        glUniform1f(glGetUniformLocation(shader.Program, "F0"), F0);
        glUniform3fv(glGetUniformLocation(shader.Program, "ambientColor"), 1, ambientColor);
        glUniform3fv(glGetUniformLocation(shader.Program, "specularColor"), 1, specularColor);

        // camera ray data
        glUniform3fv(glGetUniformLocation(shader.Program, "rayOrigin"), 1, glm::value_ptr(camera.CameraRay.origin));
        glUniform3fv(glGetUniformLocation(shader.Program, "rayDir"), 1, glm::value_ptr(camera.CameraRay.direction));

        // update normal matrix of the model basing on current view matrix
        //normalmatrix = glm::inverseTranspose(glm::mat3(view * modelMatrix));

        // transform matrices
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "modelMatrix"), 1, GL_FALSE, glm::value_ptr(modelMatrix));
        glUniformMatrix4fv(glGetUniformLocation(shader.Program, "normalMatrix"), 1, GL_FALSE, glm::value_ptr(normalmatrix));

        // default stage: rendering
        glUniform1i(glGetUniformLocation(shader.Program, "stage"), 2);

        // sculpting params
        glUniform1f(glGetUniformLocation(shader.Program, "radius"), radius);
        glUniform1f(glGetUniformLocation(shader.Program, "strength"), strength);

        #pragma endregion UNIFORMS

        if (brush) // BRUSHING BY TRANSFORM FEEDBACK
        {
            // Brush stage
            glUniform1i(glGetUniformLocation(shader.Program, "stage"), 1);
            // disabling the rasterization during brush stage
            glEnable(GL_RASTERIZER_DISCARD);
            // setting the target buffer of transform feedback computations
            glBindTransformFeedback(GL_TRANSFORM_FEEDBACK, feedback[drawBuf]);

            // start
            glBeginTransformFeedback(GL_TRIANGLES);
            glBindVertexArray(VAOs[1 - drawBuf]);
            glDrawArrays(GL_TRIANGLES, 0, model.meshes[0].vertices.size()); // drawing vertices from each primitive (triangle)
            glBindVertexArray(0);
            glEndTransformFeedback();
            glDisable(GL_RASTERIZER_DISCARD);
            // end

            // Rendering + new intersection stage
            glUniform1i(glGetUniformLocation(shader.Program, "stage"), 2);
            //TODO: the intersection shader can't see -1 index when camera ray doesn't hit the model, check why
            model.Draw(VAOs[drawBuf]);

            // swap buffers for ping ponging
            drawBuf = 1 - drawBuf;
        }
        else
        {
            // else i simply draw the model from the last computed buffer <- the drawBuf variable is not updated here
            model.Draw(VAOs[1 - drawBuf]);
        }

        //UnitCube.Draw();

        // gui cleaning
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        // swap between back and front buffer
        glfwSwapBuffers(window);
    }

    // when I exit from the graphics loop, it is because the application is closing
    // we delete the Shader Programs
    shader.Delete();

    // gui delete
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // we close and delete the created context
    glfwTerminate();
    return 0;
}

#pragma region CALLBACKS

//////////////////////////////////////////
// If one of the WASD keys is pressed, the camera is moved accordingly (the code is in utils/camera.h)
void apply_camera_movements()
{
    if(keys[GLFW_KEY_W])
        camera.ProcessKeyboard(FORWARD, deltaTime);
    if(keys[GLFW_KEY_S])
        camera.ProcessKeyboard(BACKWARD, deltaTime);
    if(keys[GLFW_KEY_A])
        camera.ProcessKeyboard(LEFT, deltaTime);
    if(keys[GLFW_KEY_D])
        camera.ProcessKeyboard(RIGHT, deltaTime);
    pointLightPosition = (camera.Position - model_pos) * 1.1f; // for light from the camera effect
}

//////////////////////////////////////////
// callback for keyboard events
void key_callback(GLFWwindow* window, int key, int scancode, int action, int mode)
{
    // if ESC is pressed, we close the application
    if(key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // if L is pressed, we activate/deactivate wireframe rendering of models
    if(key == GLFW_KEY_L && action == GLFW_PRESS)
        wireframe=!wireframe;

    // we keep trace of the pressed keys
    // with this method, we can manage 2 keys pressed at the same time:
    // many I/O managers often consider only 1 key pressed at the time (the first pressed, until it is released)
    // using a boolean array, we can then check and manage all the keys pressed at the same time
    if(action == GLFW_PRESS)
        keys[key] = true;
    else if(action == GLFW_RELEASE)
        keys[key] = false;
}

//////////////////////////////////////////
// callback for mouse events
void mouse_callback(GLFWwindow* window, double xpos, double ypos)
{
      // we move the camera view following the mouse cursor
      // we calculate the offset of the mouse cursor from the position in the last frame
      // when rendering the first frame, we do not have a "previous state" for the mouse, so we set the previous state equal to the initial values (thus, the offset will be = 0)

    if (firstMouse)
    {
        lastX = xpos;
        lastY = ypos;
        firstMouse = false;
    }

    // offset of mouse cursor position
    GLfloat xoffset = xpos - lastX;
    GLfloat yoffset = lastY - ypos;

    // the new position will be the previous one for the next frame
    lastX = xpos;
    lastY = ypos;

    // using mouse offset to move the model (0.2f is the sensitivity -> add a parameter)
    xoffset *= 0.01f;
    yoffset *= 0.01f;
    if (rotation)
    {
        // the rotation is applied to the model but not to the camera position -> rotation axis remains fixed and it not rotate with the model
        // otherwise the rotation axis is always in the same direction compared to the model rotation
        glm::vec4 inverseUp = glm::inverse(modelMatrix) * glm::vec4(camera.Up.x, camera.Up.y, camera.Up.z, 1.0f);
        glm::vec4 inverseRight = glm::inverse(modelMatrix) * glm::vec4(camera.Right.x, camera.Right.y, camera.Right.z, 1.0f);
        modelMatrix = glm::rotate(modelMatrix, xoffset, glm::vec3(inverseUp.x, inverseUp.y, inverseUp.z));
        modelMatrix = glm::rotate(modelMatrix, -yoffset, glm::vec3(inverseRight.x, inverseRight.y, inverseRight.z));
    }

    // we pass the offset to the Camera class instance in order to update the rendering <- only if we want to move the camera with the mouse cursor
    //camera.ProcessMouseMovement(xoffset, yoffset);

}

//////////////////////////////////////////
// callback for mouse key inputs
void mouse_key_callback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
        brush = true;
    else
        brush = false;

    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
        rotation = true;
    else
        rotation = false;
}

#pragma endregion CALLBACKS

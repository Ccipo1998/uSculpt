/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Vertex shader for:
- GGX illumination models

N. B.) the shader treats a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

author: Andrea Cipollini; based on RTGP course code by prof. Davide Gadia
*/

#version 460 core

// Vertex attributes in VAO

// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class
// vertex position in world coordinates
layout (location = 0) in vec3 Position;
// vertex normal in world coordinate
layout (location = 1) in vec3 Normal;
// u,v texture coordinates of the model
layout (location = 2) in vec2 TextCoords;
// tangent direction in world coordinates
layout (location = 3) in vec3 Tangent;
// bitangent direction in world coordinates
layout (location = 4) in vec3 Bitangent;

// Uniforms

// model matrix
uniform mat4 ModelMatrix;
// view matrix
uniform mat4 ViewMatrix;
// projection matrix
uniform mat4 ProjectionMatrix;
// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat4 NormalMatrix;
// the position of the point light is passed as uniform -> ( N. B.) with more lights, and of different kinds, the shader code must be modified with a for cycle, with different treatment of the source lights parameters (directions, position, cutoff angle for spot lights, etc)
uniform vec3 PointLightPosition;

void main()
{
    // vertex transformations to Canonical View Volume

    // model matrix application to the vertex -> for any transformation applied to the model (scale, translate, ...)
    vec4 mPosition = ModelMatrix * vec4( Position, 1.0 );

    // vertex position in View Coordinates
    vec4 mvPosition = ViewMatrix * mPosition;

    // vertex position in Normalized Device Coordinates (projection transformation)
    gl_Position = ProjectionMatrix * mvPosition;

    // normal, tangent and bitangent transformations in View Coordinates
    
}

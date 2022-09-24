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

struct Intersection
{
    float[3] Position;
    float[3] Normal;
    bool hit;
    uint v0, v1, v2;
};

// Vertex attributes in VAO

// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class
// vertex position in world coordinates
layout (location = 0) in vec3 Position;
// vertex normal in world coordinate
layout (location = 1) in vec3 Normal;
// u,v texture coordinates of the model
layout (location = 2) in vec2 TexCoords;
// tangent direction in world coordinates
layout (location = 3) in vec3 Tangent;
// bitangent direction in world coordinates
layout (location = 4) in vec3 Bitangent;
// neighbours
layout (location = 5) in uint NeighboursIndex;
layout (location = 6) in uint NeighboursNumber;

layout(std430, binding = 2) buffer IntersectionDataOutput
{
    Intersection IntersectionData;
};

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
// Radius of the current brush
uniform float Radius;

// outputs to fragment shader

// vertex position in View Coordinates
out vec3 vPosition;
// light direction in View Coordinates
out vec3 vLightDir;
// vertex normal transformed for fragment shader
out vec3 fNormal;
// vertex tangent in View Coordinates
out vec3 vTangent;
// vertex bitangent in View Coordinates
out vec3 vBitangent;
// texture coordinates
out vec2 fTexCoords;
// color for the intersected triangle
out vec3 hitColor;

void main()
{
    // vertex transformations to Canonical View Volume

    // model matrix application to the vertex -> for any transformation applied to the model (scale, translate, ...)
    vec4 mPosition = ModelMatrix * vec4( Position, 1.0 );

    // vertex position in View Coordinates
    vec4 mvPosition = ViewMatrix * mPosition;

    // vertex position in Normalized Device Coordinates (projection transformation)
    gl_Position = ProjectionMatrix * mvPosition;

    // light direction in View Coordinates
    vec4 vPointLightPosition = ViewMatrix * vec4(PointLightPosition, 1.0);
    vLightDir = vPointLightPosition.xyz - mvPosition.xyz;

    // position passed in View Coordinates to fragment shader
    vPosition = -mvPosition.xyz;

    // normal, tangent and bitangent -> application of model transformations
    //vNormal = (ViewMatrix * (ModelMatrix * vec4(Normal, 1.0))).xyz;
    fNormal = normalize((ModelMatrix * NormalMatrix * vec4(Normal, 1.0)).xyz);
    //vTangent = ...
    //vBitangent = ...
    
    // texture coordinates simply passed to fragment shader
    fTexCoords = TexCoords;

    vec3 interPosition = vec3(IntersectionData.Position[0], IntersectionData.Position[1], IntersectionData.Position[2]);
    float dist = distance(Position, interPosition);
    if (IntersectionData.hit && dist <= (Radius / 100 * 80))
        hitColor = vec3(1.0, 0.0, 0.0);
    else
        hitColor = vec3(0.0, 0.0, 0.0);

    /*
    // check if the vertex belongs to the intersected triangle
    if (IntersectionData.hit && (IntersectionData.v0 == gl_VertexID || IntersectionData.v1 == gl_VertexID || IntersectionData.v2 == gl_VertexID))
        hitColor = vec3(1.0, 0.0, 0.0);
    else
        hitColor = vec3(0.0, 0.0, 0.0);
    */
}

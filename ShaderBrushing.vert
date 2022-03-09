/*
09_illumination_models.vert: Vertex shader for the Lambert, Phong, Blinn-Phong and GGX illumination models

N. B.) the shader treats a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

author: Davide Gadia

Real-Time Graphics Programming - a.a. 2020/2021
Master degree in Computer Science
Universita' degli Studi di Milano

*/

#version 460 core

// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class
layout (location = 2) in vec2 TextCoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;

// input ssbo for intersection
layout (std430, binding = 1) buffer intersectionSSBO
{
    vec3 point;
    vec3 normal;
    int primitiveIndex;
} intersection;

// model matrix
uniform mat4 modelMatrix;
// view matrix
uniform mat4 viewMatrix;
// Projection matrix
uniform mat4 projectionMatrix;

// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat4 normalMatrix;

// the position of the point light is passed as uniform
// N. B.) with more lights, and of different kinds, the shader code must be modified with a for cycle, with different treatment of the source lights parameters (directions, position, cutoff angle for spot lights, etc)
uniform vec3 pointLightPosition;

uniform int stage;

// sculpting params
uniform float radius;
uniform float strength;

// light incidence direction (in view coordinates)
//out vec3 lightDir;
// the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
//out vec3 vNormal;

// in the subroutines in fragment shader where specular reflection is considered, 
// we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
//out vec3 vViewPosition;

//out vec2 textCoords;

out VS_OUT {
  vec3 position;
  vec3 lightDir;
  vec3 vNormal;
  vec3 vViewPosition;
  vec2 textCoords;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
} vs_out;


float GaussianDistribution(vec3 origin, vec3 position, float stdDev, float scaleFactor, float strength, float radius)
{
  float pi = 3.1415926535;

  float scaledStrength = strength / radius;
  float N = 1.0 / (((stdDev * scaledStrength) *
                  (stdDev * scaledStrength) *
                  (stdDev * scaledStrength)) *
                  sqrt((2.0 * pi) * (2.0 * pi) * (2.0 * pi)));
  float dx = (origin.x - position.x) * scaleFactor;
  float dy = (origin.y - position.y) * scaleFactor;
  float dz = (origin.z - position.z) * scaleFactor;
  float E = ((dx * dx) + (dy * dy) + (dz * dz)) /
            (2.0 * stdDev * stdDev);
  return N * exp(-E);
}

float NewGaussianDistribution(vec3 origin, vec3 position, float strength, float radius)
{
  float pi = 3.1415926535;
  float stdDev = 1.5;

  float N = 1.0 / ((stdDev * stdDev * stdDev) * sqrt((2.0 * pi) * (2.0 * pi) * (2.0 * pi)));
  N = N * (strength * 0.2);
  float dx = (origin.x - position.x) * 4.0 / radius;
  float dy = (origin.y - position.y) * 4.0 / radius;
  float dz = (origin.z - position.z) * 4.0 / radius;
  float E = ((dx * dx) + (dy * dy) + (dz * dz)) / (2 * stdDev * stdDev);
  
  return N * exp(-E);
}

float RadiusOffset(float radius, float strength)
{
  return 0.2; // TODO: aggiungere metodo per aggiungere un po' di offset al raggio in base al raggio corrente e a strength
}

void GaussianBrush()
{
  // here we work in world coordinates
  if (intersection.primitiveIndex != -1 && length(position - (inverse(modelMatrix) * vec4(intersection.point, 1.0)).xyz) < (radius + RadiusOffset(radius, strength)))
  {
    // we have the intersection and the current vertex is inside the radius of the stroke
    //vs_out.position = position + intersection.normal * GaussianDistribution(intersection.point, position, 0.7, 3.5 / 0.5, 2, 0.5);
    vs_out.position = position + (inverse(modelMatrix) * vec4(intersection.normal, 1.0)).xyz * NewGaussianDistribution((inverse(modelMatrix) * vec4(intersection.point, 1.0)).xyz, position, strength, radius);
  }
  else
  {
    // the new position is the same as the previous
    vs_out.position = position;
  }

  //newPosition = position + normal * 0.1;
  vs_out.normal = normal;
  vs_out.textCoords = TextCoords;
  vs_out.tangent = tangent;
  vs_out.bitangent = bitangent;
}

void main(){
  if (stage == 1)
  {
    GaussianBrush();
  }
  else
  {
    vs_out.position = (modelMatrix * vec4(position, 1.0)).xyz;
    vs_out.normal = normal;
    vs_out.textCoords = TextCoords;
    vs_out.tangent = tangent;
    vs_out.bitangent = bitangent;
  }

  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );
  
  // view direction, negated to have vector from the vertex to the camera
  vs_out.vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vs_out.vNormal = normalize((normalMatrix * vec4(normal, 1.0)).xyz);

  // light incidence direction (in view coordinate)
  vec4 lightPos = viewMatrix  * vec4(pointLightPosition, 1.0);
  vs_out.lightDir = lightPos.xyz - mvPosition.xyz;

  // sending the vertex position in world coordinates to geometry shader for intersection test

  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;

}

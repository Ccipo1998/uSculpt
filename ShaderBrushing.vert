/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Vertex shader for:
- mesh gaussian brushing
- GGX illumination models

N. B.) the shader treats a simplified situation, with a single point light.
For more point lights, a for cycle is needed to sum the contribution of each light
For different kind of lights, the computation must be changed (for example, a directional light is defined by the direction of incident light, so the lightDir is passed as uniform and not calculated in the shader like in this case with a point light).

author: Andrea Cipollini; based on RTGP course code by prof. Davide Gadia
*/

#version 460 core

// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class
// vertex position in world coordinates
layout (location = 0) in vec3 position;
// vertex normal in world coordinate
layout (location = 1) in vec3 normal;
// u,v texture coordinates of the model
layout (location = 2) in vec2 TextCoords;
// tangent direction in world coordinates
layout (location = 3) in vec3 tangent;
// bitangent direction in world coordinates
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
// projection matrix
uniform mat4 projectionMatrix;
// normals transformation matrix (= transpose of the inverse of the model-view matrix)
uniform mat4 normalMatrix;

// the position of the point light is passed as uniform
// N. B.) with more lights, and of different kinds, the shader code must be modified with a for cycle, with different treatment of the source lights parameters (directions, position, cutoff angle for spot lights, etc)
uniform vec3 pointLightPosition;

// computation stage (brush or rendering)
uniform int stage;

// sculpting params
uniform float radius;
uniform float strength;

// output to geometry shader (for both brushing and rendering) <- for each purpouse will be used different parameters from these
out VS_OUT {
  vec3 position;
  // light incidence direction (in view coordinates)
  vec3 lightDir;
  // the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
  vec3 vNormal;
  // in the subroutines in fragment shader where specular reflection is considered, 
// we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
  vec3 vViewPosition;
  vec2 textCoords;
  vec3 normal;
  vec3 tangent;
  vec3 bitangent;
} vs_out;

// Gaussian Distribution function applied to a pair of vertices with distribution height = strength and distribution "range" = radius
float GaussianDistribution(vec3 origin, vec3 position, float strength, float radius)
{
  float pi = 3.1415926535;
  float stdDev = 1.5;

  float N = 1.0 / ((stdDev * stdDev * stdDev) * sqrt((2.0 * pi) * (2.0 * pi) * (2.0 * pi)));
  N = N * (strength * 0.2 * radius);
  float dx = (origin.x - position.x) * 4.0 / radius;
  float dy = (origin.y - position.y) * 4.0 / radius;
  float dz = (origin.z - position.z) * 4.0 / radius;
  float E = ((dx * dx) + (dy * dy) + (dz * dz)) / (2 * stdDev * stdDev);
  
  return N * exp(-E);
}

// to avoid bad net deformations around the brushing action area
float RadiusOffset(float radius, float strength)
{
  return 0.2; // TODO: make a method to add offset to the brushing action ray based on current selected ray and strength
}

// apply the new position to the current vertex to obtain gaussian distribution function shape on the mesh
void GaussianBrush()
{
  // here we work in world coordinates

  // radius length + offset
  float expandedRadius = radius + RadiusOffset(radius, strength);
  // current intersection normal without transformations added
  vec3 staticCurrentIntersectionNormal = (inverse(modelMatrix) * vec4(intersection.normal, 1.0)).xyz;
  // current intersection point without transformations added
  vec3 staticCurrentIntersectionPoint = (inverse(modelMatrix) * vec4(intersection.point, 1.0)).xyz;

  // position updated if a primitive is hitted and the current vertex is in the brushing action area
  if (intersection.primitiveIndex != -1 && length(position - staticCurrentIntersectionPoint) < (expandedRadius))
  {
    // we have the intersection and the current vertex is inside the radius of the stroke
    vs_out.position = position + staticCurrentIntersectionNormal * GaussianDistribution(staticCurrentIntersectionPoint, position, strength, radius);
  }
  else
  {
    // the new position is the same as the previous one
    vs_out.position = position;
  }

  // unchanged parameters
  vs_out.normal = normal;
  vs_out.textCoords = TextCoords;
  vs_out.tangent = tangent;
  vs_out.bitangent = bitangent;
}

void main(){
  if (stage == 1)
  {
    // brushing stage
    GaussianBrush();
  }
  else
  {
    // rendering stage
    // we simply copy the data
    vs_out.position = (modelMatrix * vec4(position, 1.0)).xyz;
    vs_out.normal = normal;
    vs_out.textCoords = TextCoords;
    vs_out.tangent = tangent;
    vs_out.bitangent = bitangent;
  }
  
  // data for both rendering and intersection test in geometry shader

  // vertex position in ModelView coordinate
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );
  
  // view direction, negated to have vector from the vertex to the camera
  vs_out.vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vs_out.vNormal = normalize((normalMatrix * vec4(normal, 1.0)).xyz);

  // light incidence direction (in view coordinate)
  vec4 lightPos = viewMatrix  * vec4(pointLightPosition, 1.0);
  vs_out.lightDir = lightPos.xyz - mvPosition.xyz;

  // we apply the projection transformation and set the output position of the vertex shader
  gl_Position = projectionMatrix * mvPosition;

}

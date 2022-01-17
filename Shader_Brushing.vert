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
layout (location = 2) in vec2 texcoords;
layout (location = 3) in vec3 tangent;
layout (location = 4) in vec3 bitangent;
// the numbers used for the location in the layout qualifier are the positions of the vertex attribute
// as defined in the Mesh class

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
uniform mat3 normalMatrix;

// the position of the point light is passed as uniform
// N. B.) with more lights, and of different kinds, the shader code must be modified with a for cycle, with different treatment of the source lights parameters (directions, position, cutoff angle for spot lights, etc)
uniform vec3 pointLightPosition;

// intersection point and primitive between camera ray and mesh
//uniform vec3 interPoint;
//uniform vec3 interNormal;
//uniform int interPrimitive;
// TODO: passare la normale al punto di intersezione per utilizzarla nel brush

// Transform Feedback parameters using "buffer ping-ponging technique"

// Render pass
uniform int stage;

// Output to transform feedback buffers (pass 1)
out vec3 newPosition;
out vec3 newNormal;
out vec2 newTexCoords;
out vec3 newTangent;
out vec3 newBitangent;

// Output to fragment shader (pass 2)
// light incidence direction (in view coordinates)
out vec3 lightDir;
// the transformed normal (in view coordinate) is set as an output variable, to be "passed" to the fragment shader
// this means that the normal values in each vertex will be interpolated on each fragment created during rasterization between two vertices
out vec3 vNormal;

// in the subroutines in fragment shader where specular reflection is considered,
// we need to calculate also the reflection vector for each fragment
// to do this, we need to calculate in the vertex shader the view direction (in view coordinates) for each vertex, and to have it interpolated for each fragment by the rasterization stage
out vec3 vViewPosition;
// Output to fragment shader end

// Transform Feedback parameters end

// Transform Feedback functions

// TODO: gaussian brush

void UniformBrush()
{
  // here we work in world coordinates
  if (intersection.primitiveIndex != -1 && length(position - intersection.point) < 0.1)
  {
    // we have the intersection and the current vertex is inside the radius of the stroke
    newPosition = position + intersection.normal * 0.1;
  }
  else
  {
    // the new position is the same as the previous
    newPosition = position;
  }

  //newPosition = position + normal * 0.1;
  newNormal = normal;
  newTexCoords = texcoords;
  newTangent = tangent;
  newBitangent = bitangent;
}

float GaussianDistribution(vec3 origin, vec3 position, float stdDev, float scaleFactor, float strength, float radius)
{
  float pi = 3.1415926535;

  float scaledStrength = strength / radius * 0.1;
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

void GaussianBrush()
{
  // here we work in world coordinates
  if (intersection.primitiveIndex != -1 && length(position - intersection.point) < 0.1)
  {
    // we have the intersection and the current vertex is inside the radius of the stroke
    newPosition = position + intersection.normal * GaussianDistribution(intersection.point, position, 0.7, 3.5 / 0.1, 2.0, 0.1);
  }
  else
  {
    // the new position is the same as the previous
    newPosition = position;
  }

  //newPosition = position + normal * 0.1;
  newNormal = normal;
  newTexCoords = texcoords;
  newTangent = tangent;
  newBitangent = bitangent;
}

// Transform Feedback functions end

// Rendering functions

void Render()
{
  // vertex position in ModelView coordinate (see the last line for the application of projection)
  // when I need to use coordinates in camera coordinates, I need to split the application of model and view transformations from the projection transformations
  vec4 mvPosition = viewMatrix * modelMatrix * vec4( position, 1.0 );

  // view direction, negated to have vector from the vertex to the camera
  vViewPosition = -mvPosition.xyz;

  // transformations are applied to the normal
  vNormal = normalize( normalMatrix * normal );

  // light incidence direction (in view coordinate)
  vec4 lightPos = viewMatrix  * vec4(pointLightPosition, 1.0);
  lightDir = lightPos.xyz - mvPosition.xyz;

  // we apply the projection transformation
  gl_Position = projectionMatrix * mvPosition;
}

// Rendering functions end

void main(){
  // call the right function basing on the current stage
  if (stage == 1)
    GaussianBrush();
  else
    Render();
}

/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Geometry Shader for:
- rendering <- rendering and intersection stage
- computing the intersection between the ray from the camera and each triangle of the mesh <- rendering and intersection stage
- apply the brushing by generating new primitives with vertex shader output vertices <- brush stage

author: Andrea Cipollini; based on RTGP course code by prof. Davide Gadia
*/

#version 460 core

// input primitive type
layout (triangles) in;
// output type and vertex count per-output-primitive
layout (triangle_strip, max_vertices = 3) out;
// input ssbo for intersection
layout (std430, binding = 1) buffer intersectionSSBO
{
    vec3 point;
    vec3 normal;
    int primitiveIndex;
} intersection;

// uniform camera ray inputs
uniform vec3 rayOrigin;
uniform vec3 rayDir;

// computation stage (brush or rendering)
uniform int stage;

// output for rendering (to fragment shader)
out vec3 lightDir;
out vec3 vNormal;
out vec3 vViewPosition;
out vec2 textCoords;
out vec3 hitColor;

// output to transform feedback
out vec3 newPosition;
out vec3 newNormal;
out vec2 newTexCoords;
out vec3 newTangent;
out vec3 newBitangent;

// inputs from vertex shader -> here they are grouped in 3-length array, which is the set of the vertices of the current primitive (a triangle) 
in VS_OUT {
    vec3 position;
    vec3 lightDir;
    vec3 vNormal;
    vec3 vViewPosition;
    vec2 textCoords;
    vec3 normal;
    vec3 tangent;
    vec3 bitangent;
} gs_in[];

// ray-triangle intersection test
// Möller–Trumbore intersection algorithm (from "Fast, Minimum Storage Ray/Triangle Intersection" paper by Tomas Möller and Ben Trumbore)
// https://cadxfem.org/inf/Fast%20MinimumStorage%20RayTriangle%20Intersection.pdf
bool RayTriangleIntersection(vec3 triangleNormal)
{
    // small value for numerical stability in directions test -> comparison between the determinant to a small interval around zero
    float epsilon = 0.0000001;

    // vertices positions of the current primitive (world coordinates)
    vec3 v0 = gs_in[0].position;
    vec3 v1 = gs_in[1].position;
    vec3 v2 = gs_in[2].position;

    // directions test (culling test)
    // if the dot is negative -> a direction faces the other one
    // else the directions are perpendicular or parallel but with the same direction (i.e. the ray intersects the back of the triangle)
    if (dot(rayDir, triangleNormal) > epsilon)
        return false;
    
    // triangle edges
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    
    // determinant calculation
    // - to check if the ray lies on the triangle plane (culling test)
    // - to calculate u
    vec3 h = cross(rayDir, e2);
    float a = dot(e1, h); // determinant
    // if determinant is near zero -> ray is on the plane of the triangle
    if (a > -epsilon && a < epsilon)
        return false;

    // inverse determinant
    float f = 1.0 / a;
    // distance from vert0 to ray origin
    vec3 s = rayOrigin - v0;
    // calculating u for test bounds
    float u = f * dot(s, h);
    // if u is outside the required range for the barycentric coordinate -> the interseciton point lies on the triangle plane but outside the triangle (culling test)
    if (u < 0.0 || u > 1.0)
        return false;

    // calculating v for test bounds
    vec3 q = cross(s, e1);
    float v = f * dot(rayDir, q);
    // if v is outside the required range for the barycentric coordinate -> the interseciton point lies on the triangle plane but outside the triangle (culling test)
    if (v < 0.0 || u + v > 1.0)
        return false;

    // in fact the requirements for barycentric coordinates are:
    // - u >= 0
    // - v >= 0
    // - u + v <= 1

    // here we have hitted the triangle surface -> we compute the parameter t which gives the distance on the ray from its origin to calculate the intersection point
    float t = f * dot(e2, q);
    if (t > epsilon)
    {
        intersection.point = rayOrigin + rayDir * t; // world coordinates
        intersection.normal = triangleNormal;
        intersection.primitiveIndex = gl_PrimitiveID;
        return true;
    }
    
    return false;
}

// compute the normal to the triangle
vec3 TriangleNormal(vec3 v0, vec3 v1, vec3 v2)
{
    return normalize(cross(v1 - v0, v2 - v0));
}

void main() {

    // brush stage
    if (stage == 1)
    {
        // generating new primitive vertices only by applying previous computed positions
        // Transform Feedback writing

        newPosition = gs_in[0].position;
        newNormal = gs_in[0].normal;
        newTexCoords = gs_in[0].textCoords;
        newTangent = gs_in[0].tangent;
        newBitangent = gs_in[0].bitangent;
        EmitVertex();

        newPosition = gs_in[1].position;
        newNormal = gs_in[1].normal;
        newTexCoords = gs_in[1].textCoords;
        newTangent = gs_in[1].tangent;
        newBitangent = gs_in[1].bitangent;
        EmitVertex();

        newPosition = gs_in[2].position;
        newNormal = gs_in[2].normal;
        newTexCoords = gs_in[2].textCoords;
        newTangent = gs_in[2].tangent;
        newBitangent = gs_in[2].bitangent;
        EmitVertex();

        EndPrimitive();
    }
    // rendering and intersection stage
    else
    {
        // intersection test
        vec3 triangleNormal = TriangleNormal(gs_in[0].position, gs_in[1].position, gs_in[2].position);
        // if the current primitive is hitted by the camera ray, it will be red colored
        vec3 color = vec3(0.0, 0.0, 0.0);
        if (RayTriangleIntersection(triangleNormal))
            color = vec3(1.0, 0.0, 0.0);
        hitColor = color;
        
        // rendering
        // it is possible to use vertex data from the built-in variable gl_in, which is the previously setted gl_position variable (so in camera coordinates)
        // at last we need to send data to the fragment shader, for each vertex generated
        gl_Position = gl_in[0].gl_Position;
        lightDir = gs_in[0].lightDir;
        vNormal = triangleNormal; // normal update only for rendering
        vViewPosition = gs_in[0].vViewPosition;
        textCoords = gs_in[0].textCoords;
        EmitVertex();

        gl_Position = gl_in[1].gl_Position;
        lightDir = gs_in[1].lightDir;
        vNormal = triangleNormal; // normal update for rendering
        vViewPosition = gs_in[1].vViewPosition;
        textCoords = gs_in[1].textCoords;
        EmitVertex();

        gl_Position = gl_in[2].gl_Position;
        lightDir = gs_in[2].lightDir;
        vNormal = triangleNormal; // normal update for rendering
        vViewPosition = gs_in[2].vViewPosition;
        textCoords = gs_in[2].textCoords;
        EmitVertex();

        EndPrimitive();
    }
}

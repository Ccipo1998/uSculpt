/*
Geometry Shader to compute the intersection between camera_ray and each triangle of the mesh of the model
*/

#version 460 core

// input primitive type
layout (triangles) in;
// output type and vertex count per-output-primitive
layout (triangle_strip, max_vertices = 3) out;

// uniform camera_ray input -> those are transformed in camera coordinates before they were passed here (in appl stage)
uniform vec3 rayOrigin;
uniform vec3 rayDir;
//uniform float rayLenght;

/*
in vec3 lightDir1;
in vec3 vNormal1;
in vec3 vViewPosition1;
in vec3 textCoords1;
*/
out vec3 lightDir;
out vec3 vNormal;
out vec3 vViewPosition;
out vec2 textCoords;
out vec3 hitColor;

in VS_OUT {
    vec3 position;
    vec3 lightDir;
    vec3 vNormal;
    vec3 vViewPosition;
    vec2 textCoords;
} gs_in[];

// it is possible to use vertex data from the built-in variable gl_in, which is the previously setted gl_position variable (so in camera coordinates)

// intersection test for each primitive
// Möller–Trumbore intersection algorithm
bool RayTriangleIntersection()
{
    float epsilon = 0.0000001;

    vec3 v0 = gs_in[0].position;
    vec3 v1 = gs_in[1].position;
    vec3 v2 = gs_in[2].position;

    vec3 nor = cross(normalize(v1 - v0), normalize(v2 - v0));
    if (dot(rayDir, nor) > epsilon)
        return false;
    
    vec3 e1 = v1 - v0;
    vec3 e2 = v2 - v0;
    vec3 h = cross(rayDir, e2);
    float a = dot(e1, h);
    if (a > -epsilon && a < epsilon)
        return false;

    float f = 1.0 / a;
    vec3 s = rayOrigin - v0;
    float u = f * dot(s, h);
    if (u < 0.0 || u > 1.0)
        return false;

    vec3 q = cross(s, e1);
    float v = f * dot(rayDir, q);
    if (v < 0.0 || u + v > 1.0)
        return false;

    float t = f * dot(e2, q);
    if (t > epsilon)
        return true;
    
    return false;
}

void main() {
    // if the current primitive is hitted by the camera ray, it will be red colored
    vec3 color = vec3(0.0, 0.0, 0.0);
    if (RayTriangleIntersection())
        color = vec3(1.0, 0.0, 0.0);

    hitColor = color;
    // at last we need to send data to the fragment shader, for each vertex generated
    gl_Position = gl_in[0].gl_Position;
    lightDir = gs_in[0].lightDir;
    vNormal = gs_in[0].vNormal;
    vViewPosition = gs_in[0].vViewPosition;
    textCoords = gs_in[0].textCoords;
    EmitVertex();

    gl_Position = gl_in[1].gl_Position;
    lightDir = gs_in[1].lightDir;
    vNormal = gs_in[1].vNormal;
    vViewPosition = gs_in[1].vViewPosition;
    textCoords = gs_in[1].textCoords;
    EmitVertex();

    gl_Position = gl_in[2].gl_Position;
    lightDir = gs_in[2].lightDir;
    vNormal = gs_in[2].vNormal;
    vViewPosition = gs_in[2].vViewPosition;
    textCoords = gs_in[2].textCoords;
    EmitVertex();

    EndPrimitive();
}

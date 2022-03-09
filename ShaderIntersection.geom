/*
Geometry Shader to compute the intersection between camera_ray and each triangle of the mesh of the model
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

// uniform camera_ray input -> those are transformed in camera coordinates before they were passed here (in appl stage)
uniform vec3 rayOrigin;
uniform vec3 rayDir;
//uniform float rayLenght;

uniform int stage;

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

// Output to transform feedback buffers (pass 1)
out vec3 newPosition;
out vec3 newNormal;
out vec2 newTexCoords;
out vec3 newTangent;
out vec3 newBitangent;

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

// it is possible to use vertex data from the built-in variable gl_in, which is the previously setted gl_position variable (so in camera coordinates)

// intersection test for each primitive
// Möller–Trumbore intersection algorithm
bool RayTriangleIntersection(vec3 triangleNormal)
{
    float epsilon = 0.0000001;

    vec3 v0 = gs_in[0].position;
    vec3 v1 = gs_in[1].position;
    vec3 v2 = gs_in[2].position;

    if (dot(rayDir, triangleNormal) > epsilon)
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
    {
        intersection.point = rayOrigin + rayDir * t;
        //intersection.normal = nor;
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

float TriangleArea(vec3 v0, vec3 v1, vec3 v2)
{
    return length(cross(v1 - v0, v2 - v0)) / 2;
}

void main() {
    // brush
    if (stage == 1)
    {
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
    // rendering
    else
    {
        
        // intersection info cleared at each frame
        //intersection.primitiveIndex = 1;
        //intersection.point = vec3(0.0, 0.0, 0.0);
        //intersection.normal = vec3(0.0, 0.0, 0.0);
        vec3 triangleNormal = TriangleNormal(gs_in[0].position, gs_in[1].position, gs_in[2].position);

        // if the current primitive is hitted by the camera ray, it will be red colored
        vec3 color = vec3(0.0, 0.0, 0.0);
        if (RayTriangleIntersection(triangleNormal))
        {
            color = vec3(1.0, 0.0, 0.0);
            intersection.normal = triangleNormal;
        }

        hitColor = color;
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

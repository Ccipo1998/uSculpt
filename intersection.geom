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
    vec3 lightDir;
    vec3 vNormal;
    vec3 vViewPosition;
    vec2 textCoords;
} gs_in[];

// it is possible to use vertex data from the built-in variable gl_in, which is the previously setted gl_position variable (so in camera coordinates)

vec3 RayPlaneIntersection(vec3 pNormal) {
    /* i can compute the intersection between the ray and the triangle plane: (Ax + By + Cz + D = 0) -> plane equation where A, B and C are the x,y,z coordinates of plane normal vector
                                                                                                                        where D is the distance of the plane from the origin
                                                                                P = O + tR -> parametric equation of the ray
        we need the t parameter such as Px, Py and Pz replaced to x, y and z in the plane formula make the equation verified
    */
    float D = dot(pNormal, gl_in[0].gl_Position.xyz);
    // t = - ((A * Ox + B * Oy + C * Oz) + D) / (A * Rx + B * Ry + C * Rz) -> with substitution method
    //   = - (dot(N, O) + D) / (dot(N, R))
    float t = - (dot(pNormal, rayOrigin) + D) / dot(pNormal, rayDir);
    // found t, i can replace its value in the ray equation to get the point on the triangle plane
    return rayOrigin + t * rayDir;

}

bool IsInsideTriangle(vec3 point, vec3 normal) {

    /*
    vec3 edge1 = (gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 Pv0 = (vec4(point, 1.0) - gl_in[0].gl_Position).xyz;
    vec3 edge2 = (gl_in[2].gl_Position - gl_in[1].gl_Position).xyz;
    vec3 Pv1 = (vec4(point, 1.0) - gl_in[1].gl_Position).xyz;
    vec3 edge3 = (gl_in[0].gl_Position - gl_in[2].gl_Position).xyz;
    vec3 Pv2 = (vec4(point, 1.0) - gl_in[2].gl_Position).xyz;

    vec3 N1 = cross(edge1, Pv0);
    float dot1 = dot(N1, normal);
    vec3 N2 = cross(edge2, Pv1);
    float dot2 = dot(N2, normal);
    vec3 N3 = cross(edge3, Pv2);
    float dot3 = dot(N3, normal);

    if ((dot1 >= 0 && dot2 >= 0 && dot3 >= 0) || (dot1 <= 0 && dot2 <= 0 && dot3 <= 0)) {
        return true;
    }

    return false;
    */
    
    vec3 edge1 = (gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 Pv0 = (vec4(point, 1.0) - gl_in[0].gl_Position).xyz;
    vec3 N = cross(edge1, Pv0);
    if (dot(N, normal) < 0) return false; // P is on the right side

    vec3 edge2 = (gl_in[2].gl_Position - gl_in[1].gl_Position).xyz;
    vec3 Pv1 = (vec4(point, 1.0) - gl_in[1].gl_Position).xyz;
    N = cross(edge2, Pv1);
    if (dot(N, normal) < 0) return false; // P is on the right side

    vec3 edge3 = (gl_in[0].gl_Position - gl_in[2].gl_Position).xyz;
    vec3 Pv2 = (vec4(point, 1.0) - gl_in[2].gl_Position).xyz;
    N = cross(edge3, Pv2);
    if (dot(N, normal) < 0) return false; // P is on the right side

    return true;
    
}

vec3 RayTriangleIntersection() {
    vec3 e1 = (gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 e2 = (gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;

    vec3 q = cross(rayDir, e2);

    float a = dot(e1, q);
    if (a > -(0.00005) && a < 0.00005) return vec3(0.0, 0.0, 0.0);

    float f = 1 / a;

    vec3 s = rayOrigin - (gl_in[0].gl_Position).xyz;
    float u = f * (dot(s, q));
    if (u < 0.0) return vec3(0.0, 0.0, 0.0);

    vec3 r = cross(s, e1);
    float v = f * (dot(rayDir, r));
    if (v < 0.0 || (u + v) > 1.0) return vec3(0.0, 0.0, 0.0);

    float t = f * (dot(e2, r));

    return rayOrigin + rayDir * t;
}

void main() {

    vec3 P = vec3(0.0, 0.0, 0.0);

    // we need to check if the current triangle intersect the ray, and in positive case we need to pass the primitive to the fragment shader to highlight it
    // we compute the plane normal
    vec3 edge1 = normalize(gl_in[1].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 edge2 = normalize(gl_in[2].gl_Position - gl_in[0].gl_Position).xyz;
    vec3 pNormal = cross(edge1, edge2); // it's normalized for cross product of versors
    // if the dot product between the plane normal and the ray direction is 0 -> ray parallel to the plane of the triangle
    // if the dot product between the plane normal and the ray direction is > 0 -> the ray is behind the plane of the triangle
    // if the dot product between the plane normal and the ray direction is < 0 -> the ray intersect the plane (< 0 because camera ray and visible triangles plane normals are opposite)
    if (dot(pNormal, rayDir) < 0) {
        
        P = RayTriangleIntersection();

        if (P != vec3(0.0, 0.0, 0.0)) {
            hitColor = vec3(1.0, 0.0, 0.0); // red
        }
        else {
            hitColor = vec3(0.0, 0.0, 0.0); // black
        }
        
        /*
        P = RayPlaneIntersection(pNormal);
        // then we need to perform half-plane tests in order to check if the point is also inside the triangle
        if (IsInsideTriangle(P, pNormal)) {
            hitColor = vec3(1.0, 0.0, 0.0); // red
        }
        else {
            hitColor = vec3(0.0, 0.0, 0.0); // black
            P = vec3(0.0, 0.0, 0.0);
        }
            */
    }
    else {
        hitColor = vec3(0.0, 0.0, 0.0); // black
        P = vec3(0.0, 0.0, 0.0);
    }
    
    // at last we need to send data to the fragment shader
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

    //gl_Position = vec4(P, 1.0);
    //EmitVertex();

    EndPrimitive();
}

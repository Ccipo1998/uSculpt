// Intersection header to handle intersections between rays and models

#pragma once

using namespace std;

#include <vector>
#include <glm/glm.hpp>
#include <utils/mesh_v1.h>

const float EPSILON = 0.0000001;

// Ray struct
struct Ray3
{
    glm::vec3 origin;
    glm::vec3 direction;
    // TODO: valutare più avanti se serve il parametro per la lunghezza
};

// the intersection struct stores the intersection point in world coordinate and the index of the hitted primitive on the mesh
// if the primitive index is equal to -1 -> there is not intersection
struct Intersection
{
    glm::vec3 point;
    int primitiveIndex;
};

////////////////////////////
// it perform the test for the intersection between mesh geometry and ray
Intersection RayMeshIntersection(Mesh* mesh, Ray3* ray)
{
    // intersection test for each primitive
    // TODO: use a BVH for optimization
    // Möller–Trumbore intersection algorithm

    Intersection inter = Intersection { glm::vec3(0.0f, 0.0f, 0.0f), -1 };

    for (int i = 0; i < mesh->indices.size(); i += 3)
    {
        glm::vec3 v0 = mesh->vertices[(mesh->indices[i])].Position;
        glm::vec3 v1 = mesh->vertices[(mesh->indices[i + 1])].Position;
        glm::vec3 v2 = mesh->vertices[(mesh->indices[i + 2])].Position;

        // first test between triangle normal and ray direction
        glm::vec3 normal = glm::cross(glm::normalize(v1 - v0), glm::normalize(v2 - v0));
        if (glm::dot(ray->direction, normal) > EPSILON)
            continue;

        glm::vec3 e1 = v1 - v0;
        glm::vec3 e2 = v2 - v0;
        glm::vec3 h = glm::cross(ray->direction, e2);
        float a = glm::dot(e1, h);
        if (a > -EPSILON && a < EPSILON)
            continue;
        
        float f = 1.0f / a;
        glm::vec3 s = ray->origin - v0;
        float u = f * glm::dot(s, h);
        if (u < 0.0f || u > 1.0f)
            continue;

        glm::vec3 q = glm::cross(s, e1);
        float v = f * glm::dot(ray->direction, q);
        if (v < 0.0f || u + v > 1.0f)
            continue;
        
        float t = f * glm::dot(e2, q);
        if (t > EPSILON)
        {
            inter.point = ray->origin + ray->direction * t;
            inter.primitiveIndex = (int) i / 3;
        }
    }

    // no intersection case
    return inter;
}
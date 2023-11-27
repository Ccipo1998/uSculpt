/*
Real-time Graphics Programming - a.a. 2021/2022
Master degree in Computer Science
Universita' degli Studi di Milano

Model class
- OBJ models loading using Assimp library
- the class converts data from Assimp data structure to a OpenGL-compatible data structure (Mesh class in mesh_v1.h)

N.B. 1)  
Model and Mesh classes follow RAII principles (https://en.cppreference.com/w/cpp/language/raii).
Model is a "move-only" class. A move-only class ensures that you always have a 1:1 relationship between the total number of resources being created and the total number of actual instantiations occurring.

N.B. 2) no texturing in this version of the class

N.B. 3) based on https://github.com/JoeyDeVries/LearnOpenGL/blob/master/includes/learnopengl/model.h

author: Andrea Cipollini; based on RTGP course code by prof. Davide Gadia and by Michael Marchesan
*/



#pragma once
using namespace std;

// we use GLM data structures to convert data in the Assimp data structures in a data structures suited for VBO, VAO and EBO buffers
#include <glm/glm.hpp>

// Assimp includes
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

// we include the Mesh class, which manages the "OpenGL side" (= creation and allocation of VBO, VAO, EBO buffers) of the loading of models
#include <usculpt/mesh.h>

// include for hash map
#include <unordered_map>

// include for sets
#include <set>

/////////////////// MODEL class ///////////////////////
class Model
{
public:
    // at the end of loading, we will have a vector of Mesh class instances
    vector<Mesh> meshes;

    //////////////////////////////////////////

    // We want Model to be a move-only class. We delete copy constructor and copy assignment
    // see:
    // https://docs.microsoft.com/en-us/cpp/cpp/constructors-cpp?view=vs-2019
    // https://en.cppreference.com/w/cpp/language/copy_constructor
    // https://en.cppreference.com/w/cpp/language/copy_assignment
    // https://www.geeksforgeeks.org/preventing-object-copy-in-cpp-3-different-ways/
    // Section 4.6 of the "A Tour in C++" book
    Model(const Model& model) = delete; //disallow copy
    Model& operator=(const Model& copy) = delete;
    
    // For the Model class, a default move constructor and move assignment is sufficient
    // see:
    // https://docs.microsoft.com/en-us/cpp/cpp/move-constructors-and-move-assignment-operators-cpp?view=vs-2019
    // https://en.cppreference.com/w/cpp/language/move_constructor
    // https://en.cppreference.com/w/cpp/language/move_assignment
    // https://www.learncpp.com/cpp-tutorial/15-1-intro-to-smart-pointers-move-semantics/
    // https://www.learncpp.com/cpp-tutorial/15-3-move-constructors-and-move-assignment/
    // Section 4.6 of the "A Tour in C++" book
    Model& operator=(Model&& move) noexcept = default;
    Model(Model&& model) = default; //internally does a memberwise std::move
    
    // constructor
    // to notice that Model class is not strictly following the Rules of 5 
    // https://en.cppreference.com/w/cpp/language/rule_of_three
    // because we are not writing a user-defined destructor.
    Model(const string& path)
    {
        this->loadModel(path);
    }

    //////////////////////////////////////////

    // model rendering: calls rendering methods of each instance of Mesh class in the vector
    void Draw(GLuint buffer, RenderingType renderingType = TRIANGLES)
    {
        for(GLuint i = 0; i < this->meshes.size(); i++)
            this->meshes[i].Draw(buffer, renderingType);
    }

    void Draw(RenderingType renderingType = TRIANGLES)
    {
        for(GLuint i = 0; i < this->meshes.size(); i++)
            this->meshes[i].Draw(renderingType);
    }

    //////////////////////////////////////////


private:

    //////////////////////////////////////////
    // loading of the model using Assimp library. Nodes are processed to build a vector of Mesh class instances
    void loadModel(string path)
    {
        // loading using Assimp
        // N.B.: it is possible to set, if needed, some operations to be performed by Assimp after the loading.
        // Details on the different flags to use are available at: http://assimp.sourceforge.net/lib_html/postprocess_8h.html#a64795260b95f5a4b3f3dc1be4f52e410
        // VERY IMPORTANT: calculation of Tangents and Bitangents is possible only if the model has Texture Coordinates
        // If they are not present, the calculation is skipped (but no error is provided in the following checks!)
        Assimp::Importer importer;
        const aiScene* scene = importer.ReadFile(path, aiProcess_Triangulate | aiProcess_JoinIdenticalVertices | aiProcess_FlipUVs | aiProcess_GenSmoothNormals | aiProcess_CalcTangentSpace);

        // check for errors (see comment above)
        if(!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) // if is Not Zero
        {
            cout << "ERROR::ASSIMP:: " << importer.GetErrorString() << endl;
            return;
        }

        // we start the recursive processing of nodes in the Assimp data structure
        this->processNode(scene->mRootNode, scene);
    }

    //////////////////////////////////////////

    // Recursive processing of nodes of Assimp data structure
    void processNode(aiNode* node, const aiScene* scene)
    {
        // we process each mesh inside the current node
        for(GLuint i = 0; i < node->mNumMeshes; i++)
        {
            // the "node" object contains only the indices to objects in the scene
            // "Scene" contains all the data. Class node is used only to point to one or more mesh inside the scene and to maintain informations on relations between nodes
            aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
            // we start processing of the Assimp mesh using processMesh method.
            // the result (an istance of the Mesh class) is added to the vector
            // we use emplace_back instead as push_back, so to have the instance created directly in the 
            // vector memory, without the creation of a temp copy.
            // https://en.cppreference.com/w/cpp/container/vector/emplace_back 
            this->meshes.emplace_back(processMesh(mesh));
        }
        // we then recursively process each of the children nodes
        for(GLuint i = 0; i < node->mNumChildren; i++)
        {
            this->processNode(node->mChildren[i], scene);
        }

    }

    //////////////////////////////////////////

    // Processing of the Assimp mesh in order to obtain an "OpenGL mesh"
    // = we create and allocate the buffers used to send mesh data to the GPU
    Mesh processMesh(aiMesh* mesh)
    {
        // data structures for vertices and indices of vertices (for faces)
        vector<Vertex> vertices;
        vector<GLuint> indices;

        // neighborhood map for vertex's neighbours
        unordered_map<glm::vec3, vector<GLuint>, GlmMap, GlmMap> Nmap;
        // indices referring to the same vertex
        unordered_map<glm::vec3, set<GLuint>, GlmMap, GlmMap> Vmap;
        vector<GLuint> neighbours;

        // scale factor for inscription in a cube of 1x1x1
        float scale_factor = this->InUnitCube(mesh);

        for(GLuint i = 0; i < mesh->mNumVertices; i++)
        {
            Vertex vertex;
            // the vector data type used by Assimp is different than the GLM vector needed to allocate the OpenGL buffers
            // I need to convert the data structures (from Assimp to GLM, which are fully compatible to the OpenGL)
            glm::vec3 vector;
            // vertices coordinates
            vector.x = mesh->mVertices[i].x * scale_factor;
            vector.y = mesh->mVertices[i].y * scale_factor;
            vector.z = mesh->mVertices[i].z * scale_factor;
            vertex.Position = vector;
            // Normals
            vector.x = mesh->mNormals[i].x;
            vector.y = mesh->mNormals[i].y;
            vector.z = mesh->mNormals[i].z;
            vertex.Normal = vector;
            // Texture Coordinates
            // if the model has texture coordinates, than we assign them to a GLM data structure, otherwise we set them at 0
            // if texture coordinates are present, than Assimp can calculate tangents and bitangents, otherwise we set them at 0 too
            if(mesh->mTextureCoords[0])
            {
                glm::vec2 vec;
                // in this example we assume the model has only one set of texture coordinates. Actually, a vertex can have up to 8 different texture coordinates. For other models and formats, this code needs to be adapted and modified.
                vec.x = mesh->mTextureCoords[0][i].x;
                vec.y = mesh->mTextureCoords[0][i].y;
                vertex.TexCoords = vec;

                // Tangents
                vector.x = mesh->mTangents[i].x;
                vector.y = mesh->mTangents[i].y;
                vector.z = mesh->mTangents[i].z;
                vertex.Tangent = vector;
                // Bitangents
                vector.x = mesh->mBitangents[i].x;
                vector.y = mesh->mBitangents[i].y;
                vector.z = mesh->mBitangents[i].z;
                vertex.Bitangent = vector;
            }
            else
            {
                vertex.TexCoords = glm::vec2(0.0f, 0.0f);
            }

            // save indices of vertex duplicates
            if (Vmap.find(vertex.Position) == Vmap.end())
            {
                Vmap[vertex.Position] = set<GLuint>();
            }
            Vmap[vertex.Position].insert(i);

            // we add the vertex to the list
            vertices.push_back(vertex);
        }
        // warning if there are not uv coords
        if (!mesh->mTextureCoords[0])
            cout << "WARNING::ASSIMP:: MODEL WITHOUT UV COORDINATES -> TANGENT AND BITANGENT ARE = 0" << endl;

        // for each face of the mesh, we retrieve the indices of its vertices , and we store them in a vector data structure
        // for each face of the mesh, we retrieve also vertex's neighbours
        for(GLuint i = 0; i < mesh->mNumFaces; i++)
        {
            aiFace face = mesh->mFaces[i];
            for(GLuint j = 0; j < face.mNumIndices; j++)
            {
                indices.push_back(face.mIndices[j]);

                // check if the position is a new one
                if (Nmap.find(vertices[face.mIndices[j]].Position) == Nmap.end())
                {
                    Nmap[vertices[face.mIndices[j]].Position] = vector<GLuint>();
                }

                // add neighbours indices to current position
                // neighbours are stored like couples of each face (ex: {1, 2, 2, 3, 3, 4, 4, 1})
                for(GLuint k = (j + 1) % face.mNumIndices; k != j; k = (k + 1) % face.mNumIndices)
                {
                    Nmap[vertices[face.mIndices[j]].Position].push_back(face.mIndices[k]);
                }

                /*
                // new vertex explored
                if (Nmap.find(indexV) == Nmap.end())
                {
                    if (Vmap.find(vertices[indexV].Position) == Vmap.end())
                    {
                        // key not present
                        Nmap[indexV] = set<GLuint>();
                    }
                    else
                    {
                        // add key and copy already known neighbours
                        Nmap[indexV] = Nmap[*Vmap[vertices[indexV].Position].begin()];
                    }
                }

                // check vertex index
                if (Vmap.find(vertices[indexV].Position) == Vmap.end())
                {
                    // key not present
                    Vmap[vertices[indexV].Position] = indexV;
                }
                else
                {
                    // get already saved index
                    indexV = Vmap[vertices[indexV].Position];
                }

                // other face indices added as current index neighbours
                for(GLuint k = (j + 1) % face.mNumIndices; k != j; k = (k + 1) % face.mNumIndices)
                {
                    int indexN = face.mIndices[k];

                    // check neighbour's index
                    if (Vmap.find(vertices[indexN].Position) != Vmap.end())
                    {
                        // get already saved index
                        indexN = Vmap[vertices[indexN].Position];
                    }

                    Nmap[indexV].insert(indexN);
                }
                */
            }
        }

        // create neighbours vector and save neighborood data for vertices
        vertices[0].NeighboursIndex = 0;
        vertices[0].NeighboursNumber = Nmap[vertices[0].Position].size();
        neighbours.insert(neighbours.end(), Nmap[vertices[0].Position].begin(), Nmap[vertices[0].Position].end());
        for (GLuint i = 1; i < vertices.size(); i++)
        {
            vertices[i].NeighboursIndex = vertices[i - 1].NeighboursIndex + vertices[i - 1].NeighboursNumber;
            vertices[i].NeighboursNumber = Nmap[vertices[i].Position].size();
            neighbours.insert(neighbours.end(), Nmap[vertices[i].Position].begin(), Nmap[vertices[i].Position].end());
        }

        /*
        // convert map in to a vector
        vertices[0].NeighboursIndex = 0;
        vertices[0].NeighboursNumber = Nmap[0].size();
        neighbours.insert(neighbours.end(), Nmap[0].begin(), Nmap[0].end());
        for (GLuint i = 1; i < vertices.size(); i++)
        {
            vertices[i].NeighboursIndex = vertices[i - 1].NeighboursIndex + vertices[i - 1].NeighboursNumber;
            vertices[i].NeighboursNumber = Nmap[i].size();
            neighbours.insert(neighbours.end(), Nmap[i].begin(), Nmap[i].end());
        }
        */
        
        // we return an instance of the Mesh class created using the vertices and faces data structures we have created above.
        return Mesh(vertices, indices, neighbours);
    }

    // setting the mesh in a cube of 1x1x1 dimensions, for consistency with the sculpting params
    // return the scale factor to inscibe the mesh in a unit cube
    float InUnitCube(aiMesh* mesh)
    {
        float maxX = numeric_limits<float>::min();
        float maxY = numeric_limits<float>::min();
        float maxZ = numeric_limits<float>::min();
        float minX = numeric_limits<float>::max();
        float minY = numeric_limits<float>::max();
        float minZ = numeric_limits<float>::max();
        for (int i = 0; i < mesh->mNumVertices; i++)
        {
            aiVector3D v = mesh->mVertices[i];
            if (v.x > maxX) maxX = v.x;
            if (v.y > maxY) maxY = v.y;
            if (v.z > maxZ) maxZ = v.z;

            if (v.x < minX) minX = v.x;
            if (v.y < minY) minY = v.y;
            if (v.z < minZ) minZ = v.z;
        }
        float extensionX = abs(maxX) + abs(minX);
        float extensionY = abs(maxY) + abs(minY);
        float extensionZ = abs(maxZ) + abs(minZ);
        float maxExtension = max({extensionX, extensionY, extensionZ});

        return 1 / maxExtension;
    }

    struct GlmMap
    {
        size_t operator()(const glm::vec3& k)const
        {
            return std::hash<float>()(k.x) ^ std::hash<float>()(k.y) ^ std::hash<float>()(k.z);
        }

        bool operator()(const glm::vec3& a, const glm::vec3& b)const
        {
            return a == b;
        }
    };
    
};

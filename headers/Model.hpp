#pragma once
#include <vector>
#include <string>
#include <assimp/scene.h>
#include <unordered_map>
#include "Mesh.hpp"
#include "Shader.hpp"

class Model {
public:
    Model(const std::string& path);
    void Draw(Shader& shader);
    size_t MeshCount() const { return meshes.size(); }
    const std::vector<Mesh>& getMeshes() const { return meshes; }
private:
    std::vector<Mesh> meshes;
    std::string directory;
    std::unordered_map<std::string, glm::vec3> materialColorOverride;
    void loadModel(std::string path);
    void processNode(aiNode* node, const aiScene* scene);
    Mesh processMesh(aiMesh* mesh, const aiScene* scene);
};
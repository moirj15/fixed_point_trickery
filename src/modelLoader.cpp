#include "modelLoader.hpp"

#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>

Model ProcessMeshes(const aiScene *scene, const std::vector<aiMesh *> &aiMeshes)
{
  Model model;
  for (auto *aiMesh : aiMeshes)
  {
    ModelMesh mesh;
    for (size_t i = 0; i < aiMesh->mNumVertices; i++)
    {
      mesh.vertices.emplace_back(ModelVertex{
        .position =
          {
            aiMesh->mVertices[i].x,
            aiMesh->mVertices[i].y,
            aiMesh->mVertices[i].z,
          },
        .normal =
          {
            aiMesh->mNormals[i].x,
            aiMesh->mNormals[i].y,
            aiMesh->mNormals[i].z,
          },
        .textureCoord =
          {
            aiMesh->mTextureCoords[0][i].y,
            aiMesh->mTextureCoords[0][i].y,
          },
      });
    }

    for (size_t faceIndex = 0; faceIndex < aiMesh->mNumFaces; faceIndex++)
    {
      const aiFace &face = aiMesh->mFaces[faceIndex];
      for (size_t i = 0; i < face.mNumIndices; i++)
      {
        mesh.indices.push_back(face.mIndices[i]);
      }
    }
    model.parts.push_back(mesh);
  }
  return model;
}

void ProcessNode(aiNode *node, const aiScene *scene, std::vector<aiMesh *> &aiMeshes)
{
  for (size_t i = 0; i < node->mNumMeshes; i++)
  {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    aiMeshes.emplace_back(mesh);
  }
  for (size_t i = 0; i < node->mNumChildren; i++)
  {
    ProcessNode(node->mChildren[i], scene, aiMeshes);
  }
}

Model LoadModel(const std::string &path)
{
  // const std::string MODEL_PATH       = "../../models";

  Assimp::Importer importer;
  const aiScene   *scene = importer.ReadFile(path, aiProcess_Triangulate);
  assert(scene);
  std::vector<aiMesh *> aiMeshes;
  ProcessNode(scene->mRootNode, scene, aiMeshes);
  return ProcessMeshes(scene, aiMeshes);
}

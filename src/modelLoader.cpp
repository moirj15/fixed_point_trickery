#include "modelLoader.hpp"

#include <assimp/BaseImporter.h>
#include <assimp/Exceptional.h>
#include <assimp/Importer.hpp>
#include <assimp/postprocess.h>
#include <assimp/scene.h>
#include <filesystem>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <print>

glm::mat4 AssimpToMat4(const aiMatrix4x4 &aiMat)
{
  glm::mat4 mat;
  // the a,b,c,d in assimp is the row ; the 1,2,3,4 is the column
  mat[0][0] = aiMat.a1;
  mat[1][0] = aiMat.a2;
  mat[2][0] = aiMat.a3;
  mat[3][0] = aiMat.a4;
  mat[0][1] = aiMat.b1;
  mat[1][1] = aiMat.b2;
  mat[2][1] = aiMat.b3;
  mat[3][1] = aiMat.b4;
  mat[0][2] = aiMat.c1;
  mat[1][2] = aiMat.c2;
  mat[2][2] = aiMat.c3;
  mat[3][2] = aiMat.c4;
  mat[0][3] = aiMat.d1;
  mat[1][3] = aiMat.d2;
  mat[2][3] = aiMat.d3;
  mat[3][3] = aiMat.d4;
  return mat;
}

Model ProcessMeshes(
  const aiScene               *scene,
  const std::vector<aiMesh *> &aiMeshes,
  std::vector<glm::mat4>     &&transforms)
{
  Model model;
  model.transforms = transforms;
  auto GetTexCoord = [](aiMesh *aiMesh, size_t i) -> glm::vec2 {
    if (aiMesh->mTextureCoords[0] != nullptr)
    {
      return {
        aiMesh->mTextureCoords[0][i].x,
        aiMesh->mTextureCoords[0][i].y,
      };
    }
    else
    {
      return {0.0f, 0.0f};
    }
  };
  for (auto *aiMesh : aiMeshes)
  {
    if (aiMesh->mPrimitiveTypes != aiPrimitiveType_TRIANGLE)
    {
      continue;
    }
    ModelMesh mesh;
    mesh.boundingBox = {
      .min =
        {
          aiMesh->mAABB.mMin.x,
          aiMesh->mAABB.mMin.y,
          aiMesh->mAABB.mMin.z,
        },
      .max =
        {
          aiMesh->mAABB.mMax.x,
          aiMesh->mAABB.mMax.y,
          aiMesh->mAABB.mMax.z,
        },
    };
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
        .textureCoord = GetTexCoord(aiMesh, i),

      });
    }
    mesh.hasTexCoord = aiMesh->mTextureCoords[0] != nullptr;

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

void ProcessNode(
  aiNode                 *node,
  const aiScene          *scene,
  std::vector<aiMesh *>  &aiMeshes,
  std::vector<glm::mat4> &transforms)
{
  glm::dmat4 transform;
  if (transforms.empty())
  {
    transform = AssimpToMat4(node->mTransformation);
  }
  else
  {
    transform = transforms.back() * AssimpToMat4(node->mTransformation);
  }
  for (size_t i = 0; i < node->mNumMeshes; i++)
  {
    aiMesh *mesh = scene->mMeshes[node->mMeshes[i]];
    aiMeshes.emplace_back(mesh);
    transforms.emplace_back(transform);
  }
  for (size_t i = 0; i < node->mNumChildren; i++)
  {
    ProcessNode(node->mChildren[i], scene, aiMeshes, transforms);
  }
}

Model LoadModel(const std::string &path)
{
  // const std::string MODEL_PATH       = "../../models";

  Assimp::Importer importer;
  // auto            *glbImporter = importer.GetImporter("glb");
  auto p = std::filesystem::current_path();
  auto e = std::filesystem::exists(path);
  // Sort by the primitive type so we can skip anything that's not an indexed triangle

  const aiScene *scene = importer.ReadFile(
    path,
    aiProcess_GenBoundingBoxes | aiProcess_Triangulate | aiProcess_SortByPType
      | aiProcess_GenUVCoords);
  if (scene == nullptr)
  {
    std::println("err: {}", importer.GetErrorString());
    std::exit(0);
  }
  assert(scene);
  std::vector<aiMesh *>  aiMeshes;
  std::vector<glm::mat4> transforms;
  ProcessNode(scene->mRootNode, scene, aiMeshes, transforms);
  return ProcessMeshes(scene, aiMeshes, std::move(transforms));
}

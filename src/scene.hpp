#pragma once

#include "utils.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>
#include <vector>

struct ModelVertex
{
  glm::vec3 position{};
  glm::vec3 normal{};
  glm::vec2 textureCoord{};
};

struct BoundingBox
{
  glm::dvec3 min{};
  glm::dvec3 max{};

  glm::vec3 GetScale() const
  {
    return (max - min) * 0.5;
  }
};

struct ModelMesh
{
  std::vector<ModelVertex> vertices;
  std::vector<u32>         indices;
  bool                     hasTexCoord{};
  BoundingBox              boundingBox;
};

struct Model
{
  std::vector<ModelMesh> parts;
  std::vector<glm::mat4> transforms;
  glm::dvec3             position{};
};

struct Scene
{
  Model model;
};
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

struct ModelMesh
{
  std::vector<ModelVertex> vertices;
  std::vector<u32>         indices;
  bool                     hasTexCoord{};
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
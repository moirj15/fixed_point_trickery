#include "CpuDouble.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

CpuDoubleMethod::CpuDoubleMethod(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(
      VERT_PATH,
      PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(ModelVertex, position),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "NORMAL",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(ModelVertex, normal),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "TEXCOORD",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(ModelVertex, textureCoord),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
      })},
    mDevice{device}
{
}

void CpuDoubleMethod::SetScene(const Scene &scene)
{
  mScene = scene;
  std::vector<u32> indices;
  u32              vertexStart = 0;
  u32              startIndex  = 0;
  for (auto &mesh : scene.model.parts)
  {
    for (auto &index : mesh.indices)
    {
      indices.push_back(index /* + indexStart*/);
    }
    mDraws.push_back({
      .startIndex{startIndex},
      .baseVertex{vertexStart},
      .indexCount{static_cast<u32>(mesh.indices.size())},
    });
    vertexStart += mesh.vertices.size();
    startIndex += mesh.indices.size();
  }
  mIndexBuf = dx::CreateIndexBuffer<uint32_t>(
    mDevice,
    indices.size(),
    std::span<uint32_t>{indices.begin(), indices.end()});

  mTransformedVertBuf = dx::CreateVertexBuffer<ModelVertex>(mDevice, mVertices.size(), {}, true);
}

void CpuDoubleMethod::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos)
{
  const glm::dmat4 mvp = cameraProjection * glm::translate(glm::identity<glm::dmat4>(), modelPos);
  D3D11_MAPPED_SUBRESOURCE mapped{};

  dx::ThrowIfFailed(
    ctx->Map(mTransformedVertBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));

  std::span<ModelVertex> gpuVertices = {reinterpret_cast<ModelVertex *>(mapped.pData), mTotalSize};

  size_t gpuIndex = 0;
  for (size_t meshIndex = 0; meshIndex < mScene.model.parts.size(); meshIndex++)
  {
    const auto      &mesh          = mScene.model.parts[meshIndex];
    const glm::dmat4 meshTransform = mScene.model.transforms[meshIndex];
    const glm::dmat4 transform     = mvp * meshTransform;
    for (const auto &vertex : mesh.vertices)
    {
      // TODO: should be better to have a seperate buffer for the positions
      gpuVertices[meshIndex] = {
        .position     = transform * glm::dvec4{vertex.position, 1.0},
        .normal       = vertex.normal,
        .textureCoord = vertex.textureCoord,
      };
      meshIndex++;
    }
  }
  ctx->Unmap(mTransformedVertBuf.Get(), 0);
}

void CpuDoubleMethod::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
}

} // namespace methods
#include "CpuDouble.hpp"

#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

struct CpuDoubleVertex
{
  glm::vec4 position{};
  glm::vec3 normal{};
  glm::vec2 textureCoord{};
};

using VertexFormat = CpuDoubleVertex;

CpuDoubleMethod::CpuDoubleMethod(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(
      VERT_PATH,
      PIXEL_PATH,
      {
        {
          .SemanticName         = "POSITION",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32A32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, position),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "NORMAL",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32B32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, normal),
          .InputSlotClass       = D3D11_INPUT_PER_VERTEX_DATA,
          .InstanceDataStepRate = 0,
        },
        {
          .SemanticName         = "TEXCOORD",
          .SemanticIndex        = 0,
          .Format               = DXGI_FORMAT_R32G32_FLOAT,
          .InputSlot            = 0,
          .AlignedByteOffset    = offsetof(VertexFormat, textureCoord),
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
  mDraws = {};
  std::vector<u32> indices;
  u32              vertexStart = 0;
  u32              startIndex  = 0;
  u32              totalSize   = 0;
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
    mTotalSize += mesh.vertices.size();
  }
  mIndexBuf = dx::CreateIndexBuffer<uint32_t>(
    mDevice,
    indices.size(),
    std::span<uint32_t>{indices.begin(), indices.end()});

  mTransformedVertBuf = dx::CreateVertexBuffer<VertexFormat>(mDevice, mTotalSize, {}, true);
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

  std::span gpuVertices = {reinterpret_cast<VertexFormat *>(mapped.pData), mTotalSize};

  size_t gpuIndex = 0;
  for (size_t meshIndex = 0; meshIndex < mScene.model.parts.size(); meshIndex++)
  {
    const auto      &mesh          = mScene.model.parts[meshIndex];
    const glm::dmat4 meshTransform = mScene.model.transforms[meshIndex];
    const glm::dmat4 transform     = mvp * meshTransform;
    for (const auto &vertex : mesh.vertices)
    {
      // TODO: should be better to have a seperate buffer for the positions
      gpuVertices[gpuIndex] = {
        .position     = transform * glm::dvec4{vertex.position, 1.0},
        .normal       = vertex.normal,
        .textureCoord = vertex.textureCoord,
      };
      gpuIndex++;
    }
  }
  ctx->Unmap(mTransformedVertBuf.Get(), 0);
}

void CpuDoubleMethod::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
  RenderProgram        rp  = shaderWatcher.GetRenderProgram(mShadersHandle);
  ID3D11DeviceContext *ctx = renderContext.DeviceContext();

  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->IASetIndexBuffer(mIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
  u32 stride = sizeof(VertexFormat);
  u32 offset = 0;
  ctx->IASetVertexBuffers(0, 1, mTransformedVertBuf.GetAddressOf(), &stride, &offset);
  ctx->IASetInputLayout(rp.inputLayout);
  ctx->VSSetShader(rp.vertexShader, nullptr, 0);

  ctx->RSSetState(renderContext.rasterizerState.Get());

  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
  ctx->OMSetRenderTargets(
    1,
    renderContext.backbufferRTV.GetAddressOf(),
    renderContext.depthStencilView.Get());
  for (u32 i = 0; i < mDraws.size(); i++)
  {
    const DrawOffsets draw = mDraws[i];
    ctx->DrawIndexed(draw.indexCount, draw.startIndex, draw.baseVertex);
  }
}

} // namespace methods
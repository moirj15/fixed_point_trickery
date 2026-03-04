#include "F32.hpp"

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>
#include <imgui.h>

namespace methods
{

struct SceneData
{
  glm::mat4 modelView;
};

struct PerMeshData
{
  glm::mat4 transform{};
  u32       vertexOffset{};
};

F32Method::F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(VERT_PATH, PIXEL_PATH)},
    mVertBuf{},
    mIndexBuf{},
    mConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)},
    mPerMeshDataBuf{},
    mDevice{device}
{
}

void F32Method::SetScene(const Scene &scene)
{
  mDraws = {};
  std::vector<ModelVertex> vertices;
  std::vector<u32>         indices;

  u32 vertexStart = 0;
  u32 startIndex  = 0;
  for (auto &mesh : scene.model.parts)
  {
    for (auto &vertex : mesh.vertices)
    {
      vertices.push_back(vertex);
    }
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
  mDrawIDBuf = dx::CreateDrawIDBuffer(mDevice, mDraws.size());
  mVertBuf   = dx::CreateStorageBuffer<ModelVertex>(
    mDevice,
    vertices.size(),
    std::span{
      vertices.begin(),
      vertices.end(),
    });

  mIndexBuf = dx::CreateIndexBuffer<uint32_t>(
    mDevice,
    indices.size(),
    std::span<uint32_t>{indices.begin(), indices.end()});

  std::vector<PerMeshData> perMeshData(mDraws.size());
  for (u32 i = 0; i < perMeshData.size(); i++)
  {
    perMeshData[i] = {
      .transform    = scene.model.transforms[i],
      .vertexOffset = mDraws[i].baseVertex,
    };
  }

  mPerMeshDataBuf = dx::CreateStorageBuffer<PerMeshData>(
    mDevice,
    scene.model.transforms.size(),
    std::span{
      perMeshData.begin(),
      perMeshData.end(),
    });
}

void F32Method::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos)
{
  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  SceneData data{
    .modelView =
      glm::mat4{cameraProjection /** glm::translate(glm::identity<glm::dmat4>(), modelPos)*/},
  };
  // SceneData data{glm::mat4(1.0f), glm::vec3(1.0)};
  memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  // glm::mat4 mvp{cameraProjection};
  // memcpy(mapped.pData, glm::value_ptr(mvp), sizeof(glm::mat4));
  ctx->Unmap(mConstantBuf.Get(), 0);
}

void F32Method::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
  RenderProgram        rp  = shaderWatcher.GetRenderProgram(mShadersHandle);
  ID3D11DeviceContext *ctx = renderContext.DeviceContext();

  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->IASetIndexBuffer(mIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
  ctx->VSSetShader(rp.vertexShader, nullptr, 0);
  ctx->VSSetShaderResources(1, 1, mDrawIDBuf.view.GetAddressOf());
  ctx->VSSetShaderResources(1, 1, mVertBuf.view.GetAddressOf());
  ctx->VSSetShaderResources(2, 1, mPerMeshDataBuf.view.GetAddressOf());
  ctx->VSSetConstantBuffers(0, 1, mConstantBuf.GetAddressOf());

  ctx->RSSetState(renderContext.rasterizerState.Get());

  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
  ctx->OMSetRenderTargets(
    1,
    renderContext.backbufferRTV.GetAddressOf(),
    renderContext.depthStencilView.Get());
  // for (u32 i = 0; i < mDraws.size(); i++)
  //{
  static i32 v;
  ImGui::SliderInt("subModel", &v, 0, mDraws.size() - 1);
  const DrawOffsets draw = mDraws[v];
  ImGui::Text("Index Count: %d", draw.indexCount);
  ImGui::Text("Start Index: %d", draw.startIndex);
  ImGui::Text("Base Vertex: %d", draw.baseVertex);
  ctx->DrawIndexedInstanced(draw.indexCount, 1, draw.startIndex, draw.baseVertex, v);
  //}
}

} // namespace methods

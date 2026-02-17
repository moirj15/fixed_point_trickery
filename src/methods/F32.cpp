#include "F32.hpp"

#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

struct SceneData
{
  glm::mat4 mvp;
  glm::vec3 color;
};

F32Method::F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(VERT_PATH, PIXEL_PATH)},
    mVertBuf{},
    mIndexBuf{},
    mConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)},
    mDevice{device}
{
}

void F32Method::SetScene(const Scene &scene)
{
  std::vector<ModelVertex> vertices;
  std::vector<u32>         indices;

  u32 indexStart = 0;
  for (auto &mesh : scene.models[0].parts)
  {
    for (auto &vertex : mesh.vertices)
    {
      vertices.push_back(vertex);
    }
    for (auto &index : mesh.indices)
    {
      indices.push_back(index + indexStart);
    }
    indexStart += mesh.vertices.size();
  }
  mVertBuf = dx::CreateVertexBuffer<ModelVertex>(
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
  mTotalDraw = indices.size();
}

void F32Method::Update(ID3D11DeviceContext3 *ctx, const glm::dmat4 &camera)
{
  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  // SceneData data{glm::mat4(1.0f), glm::vec3(1.0)};
  // memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  glm::mat4 mvp{camera};
  memcpy(mapped.pData, glm::value_ptr(mvp), sizeof(glm::mat4));
  ctx->Unmap(mConstantBuf.Get(), 0);
}

void F32Method::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
  RenderProgram        rp  = shaderWatcher.GetRenderProgram(mShadersHandle);
  ID3D11DeviceContext *ctx = renderContext.DeviceContext();

  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->IASetIndexBuffer(mIndexBuf.Get(), DXGI_FORMAT_R32_UINT, 0);
  ctx->VSSetShader(rp.vertexShader, nullptr, 0);
  ctx->VSSetShaderResources(0, 1, mVertBuf.view.GetAddressOf());
  ctx->VSSetConstantBuffers(0, 1, mConstantBuf.GetAddressOf());

  ctx->RSSetState(renderContext.rasterizerState.Get());

  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
  ctx->OMSetRenderTargets(
    1,
    renderContext.backbufferRTV.GetAddressOf(),
    renderContext.depthStencilView.Get());
  ctx->DrawIndexed(mTotalDraw, 0, 0);
}

} // namespace methods

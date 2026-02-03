#include "F32.hpp"

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace methods
{

struct Vertex
{
  glm::vec3 pos;
};

struct SceneData
{
  glm::mat4 mvp;
  glm::vec3 color;
};

std::array<Vertex, 3> v{
  glm::vec3{0, 1, -1},
  glm::vec3{-1, 0, -1},
  glm::vec3{1, 0, -1},
};

F32Method::F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(VERT_PATH, PIXEL_PATH)},
    mVertBuf{dx::CreateVertexBuffer<Vertex>(device, sizeof(Vertex) * 3, sizeof(Vertex), v)},
    mConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)}
{
}

void F32Method::Update(ID3D11DeviceContext3 *ctx)
{
  // NOP for now
  D3D11_MAPPED_SUBRESOURCE mapped{};
  dx::ThrowIfFailed(ctx->Map(mConstantBuf.Get(), 0, D3D11_MAP_WRITE_NO_OVERWRITE, 0, &mapped));
  SceneData data{glm::mat4(1.0f), glm::vec3(1.0)};
  memcpy(mapped.pData, (void *)&data, sizeof(SceneData));
  ctx->Unmap(mConstantBuf.Get(), 0);
}

void F32Method::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
  RenderProgram        rp  = shaderWatcher.GetRenderProgram(mShadersHandle);
  ID3D11DeviceContext *ctx = renderContext.DeviceContext();

  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->VSSetShader(rp.vertexShader, nullptr, 0);
  ctx->VSSetShaderResources(0, 1, mVertBuf.view.GetAddressOf());
  ctx->VSSetConstantBuffers(0, 1, mConstantBuf.GetAddressOf());

  ctx->RSSetState(renderContext.rasterizerState.Get());

  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
  ctx->OMSetRenderTargets(
    1,
    renderContext.backbufferRTV.GetAddressOf(),
    renderContext.depthStencilView.Get());
  ctx->Draw(3, 0);
}

} // namespace methods

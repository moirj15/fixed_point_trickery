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
  glm::vec3{0, 1, 0},
  glm::vec3{-1, 0, 0},
  glm::vec3{1, 0, 0},
};

F32Method::F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(VERT_PATH, PIXEL_PATH)},
    mVertBuf{dx::CreateVertexBuffer<Vertex>(device, sizeof(Vertex) * 3, 0, v)},
    mConstantBuf{dx::CreateConstantBuffer<SceneData>(device, nullptr)}
{
}

void F32Method::Update(ID3D11DeviceContext3 *ctx)
{
  // NOP for now
}

void F32Method::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
  RenderProgram        rp  = shaderWatcher.GetRenderProgram(mShadersHandle);
  ID3D11DeviceContext *ctx = renderContext.DeviceContext();

  ctx->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx->VSSetShader(rp.vertexShader, nullptr, 0);
  ctx->VSGetShaderResources(0, 1, mVertBuf.view.GetAddressOf());
  ctx->VSSetConstantBuffers(0, 1, mConstantBuf.GetAddressOf());
  ctx->RSSetState(renderContext.rasterizerState.Get());
  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
}

} // namespace methods

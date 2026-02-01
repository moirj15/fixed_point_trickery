#include "F32.hpp"

#include <array>
#include <glm/vec3.hpp>

namespace methods
{

struct Vertex
{
  glm::vec3 pos;
};

std::array<Vertex, 3> v{
  glm::vec3{0, 1, 0},
  glm::vec3{-1, 0, 0},
  glm::vec3{1, 0, 0},
};

F32Method::F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher) :
    mShadersHandle{shaderWatcher.RegisterShader(VERT_PATH, PIXEL_PATH)},
    mVertBuf{dx::CreateVertexBuffer<Vertex>(device, sizeof(Vertex) * 3, 0, v)}
{
}

void F32Method::Update(ID3D11DeviceContext3 *ctx)
{
  // NOP for now
}

void F32Method::Draw(ID3D11DeviceContext3 *ctx, ShaderWatcher &shaderWatcher)
{
  RenderProgram rp = shaderWatcher.GetRenderProgram(mShadersHandle);

  ctx->VSSetShader(rp.vertexShader, nullptr, 0);
  ctx->PSSetShader(rp.pixelShader, nullptr, 0);
}

} // namespace methods

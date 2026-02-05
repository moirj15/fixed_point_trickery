#pragma once

#include "../dx.hpp"
#include "../scene.hpp"
#include "../shaderWatcher.hpp"

#include <glm/mat4x4.hpp>

namespace methods
{

class F32Method final
{
  const std::string VERT_PATH  = "shaders/F32.hlsl";
  const std::string PIXEL_PATH = "shaders/F32.hlsl";

  RenderProgramHandle  mShadersHandle;
  dx::VertexBuffer     mVertBuf;
  ComPtr<ID3D11Buffer> mConstantBuf;
  Scene                mScene;

public:
  explicit F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher, const Scene &scene);

  void Update(ID3D11DeviceContext3 *ctx, const glm::dmat4 &camera);

  void Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher);
};

} // namespace methods
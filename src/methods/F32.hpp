#pragma once

#include "../dx.hpp"
#include "../shaderWatcher.hpp"

namespace methods
{

class F32Method final
{
  const std::string VERT_PATH  = "src/shaders/F32.hlsl";
  const std::string PIXEL_PATH = "src/shaders/F32.hlsl";

  RenderProgramHandle  mShadersHandle;
  dx::VertexBuffer     mVertBuf;
  ComPtr<ID3D11Buffer> mConstantBuf;

public:
  explicit F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher);

  void Update(ID3D11DeviceContext3 *ctx);

  void Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher);
};

} // namespace methods
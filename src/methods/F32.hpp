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

  struct DrawOffsets
  {
    u32 indexOffset{};
    u32 startVertex{};
    u32 indexCount{};
  };

  RenderProgramHandle      mShadersHandle;
  dx::StorageBuffer        mVertBuf;
  ComPtr<ID3D11Buffer>     mIndexBuf;
  ComPtr<ID3D11Buffer>     mConstantBuf;
  dx::StorageBuffer        mTransformsBuf;
  std::vector<DrawOffsets> mDraws;
  Scene                    mScene{};
  ID3D11Device3           *mDevice;

public:
  explicit F32Method(ID3D11Device3 *device, ShaderWatcher &shaderWatcher);

  void SetScene(const Scene &scene);

  void
  Update(ID3D11DeviceContext3 *ctx, const glm::dmat4 &cameraProjection, const glm::dvec3 &modelPos);

  void Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher);
};

} // namespace methods
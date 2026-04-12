#pragma once

#include "../dx.hpp"
#include "../scene.hpp"
#include "../shaderWatcher.hpp"

#include <array>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct Scene;
struct RenderContext;

namespace methods
{

class CpuDoubleMethod final
{
  const std::string VERT_PATH  = "shaders/CpuDouble.hlsl";
  const std::string PIXEL_PATH = "shaders/CpuDouble.hlsl";

  struct DrawOffsets
  {
    u32 startIndex{};
    u32 baseVertex{};
    u32 indexCount{};
  };

  RenderProgramHandle      mShadersHandle;
  ComPtr<ID3D11Buffer>     mTransformedVertBuf;
  ComPtr<ID3D11Buffer>     mIndexBuf;
  std::vector<DrawOffsets> mDraws;

  std::vector<std::vector<ModelVertex>> mMeshes;
  u32                                   mTotalSize{};

  Scene          mScene{};
  ID3D11Device3 *mDevice;

  std::array<ComPtr<ID3D11Query>, 60> mGpuDisjointQueries;
  std::array<ComPtr<ID3D11Query>, 60> mGpuStarts;
  std::array<ComPtr<ID3D11Query>, 60> mGpuEnds;

public:
  explicit CpuDoubleMethod(ID3D11Device3 *device, ShaderWatcher &shaderWatcher);

  void SetScene(const Scene &scene);

  void
  Update(ID3D11DeviceContext3 *ctx, const glm::dmat4 &cameraProjection, const glm::dvec3 &modelPos);

  void Draw(
    dx::RenderContext      &renderContext,
    ShaderWatcher          &shaderWatcher,
    ID3D11RenderTargetView *targetView,
    bool                    recordDrawTime,
    u32                     testFrameCount);

  std::vector<double> GetTimingData(ID3D11DeviceContext3 *ctx);
};

} // namespace methods

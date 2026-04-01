#pragma once

#include "../arcball_camera.h"
#include "../dx.hpp"
#include "../scene.hpp"
#include "../shaderWatcher.hpp"

#include <glm/mat3x3.hpp>
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct Scene;
struct RenderContext;

namespace methods
{

class Parallax final
{
  const std::string VERT_PATH  = "shaders/Parallax.hlsl";
  const std::string PIXEL_PATH = "shaders/Parallax.hlsl";

  const std::string BB_DEBUG_VERT_PATH  = "shaders/BoundingBox.hlsl";
  const std::string BB_DEBUG_PIXEL_PATH = "shaders/BoundingBox.hlsl";

  const std::string BB_TEXTURED_VERT_PATH  = "shaders/BoundingBoxTextured.hlsl";
  const std::string BB_TEXTURED_PIXEL_PATH = "shaders/BoundingBoxTextured.hlsl";

  struct DrawOffsets
  {
    u32 startIndex{};
    u32 baseVertex{};
    u32 indexCount{};
  };

  RenderProgramHandle               mShadersHandle;
  dx::StorageBuffer                 mDrawIDBuf;
  ComPtr<ID3D11Buffer>              mVertBuf;
  ComPtr<ID3D11Buffer>              mIndexBuf;
  ComPtr<ID3D11Buffer>              mConstantBuf;
  std::vector<DrawOffsets>          mDraws;
  std::vector<ComPtr<ID3D11Buffer>> mModelConstants;
  Scene                             mScene{};
  ID3D11Device3                    *mDevice;

  RenderProgramHandle               mBBDebugShadersHandle;
  ComPtr<ID3D11Buffer>              mBBDebugVertBuf;
  ComPtr<ID3D11Buffer>              mBBDebugIndexBuf;
  ComPtr<ID3D11Buffer>              mBBDebugConstantBuf;
  std::vector<ComPtr<ID3D11Buffer>> mBBDebugModelConstants;
  u32                               mBBDebugIndexCount;
  ComPtr<ID3D11RasterizerState>     mBBDebugRSState;
  std::vector<glm::dmat4>           mBBTransforms;

  ComPtr<ID3D11Texture2D>          mImposterCubeTexture;
  ComPtr<ID3D11ShaderResourceView> mImposterTextureView;
  ComPtr<ID3D11RenderTargetView>   mImposterTarget;
  ComPtr<ID3D11Texture2D>          mImposterDepthBuffer;
  ComPtr<ID3D11DepthStencilView>   mImposterDepthView;
  ComPtr<ID3D11Buffer>             mImposterTargetCB;

  ComPtr<ID3D11SamplerState>       mImposterSamplerState;
  ComPtr<ID3D11ShaderResourceView> mImposterDepthTexView;

  RenderProgramHandle mBBTexShaderHandle;

public:
  explicit Parallax(ID3D11Device3 *device, ShaderWatcher &shaderWatcher);

  void SetScene(const Scene &scene);

  void Draw(
    u32                   width,
    u32                   height,
    ID3D11DeviceContext3 *ctx,
    const glm::dmat4     &cameraProjection,
    const glm::dmat4     &camera,
    const glm::dmat4     &projection,
    const glm::dvec3     &modelPos,
    const glm::dvec3     &cameraPos,
    const ArcballCamera  &arcball,
    dx::RenderContext    &renderContext,
    ShaderWatcher        &shaderWatcher);
};

} // namespace methods

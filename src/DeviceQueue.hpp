#pragma once
#include "dx.hpp"

namespace kronk
{

class RenderCommandBuffer final
{
public:
  void BeginPass();

  void SetPipeline(const RenderPipeline &pipeline);

  void Draw(u32 vertexCount, u32 startVertex);

  void EndPass();
};

class ComputeCommandBuffer final
{
public:
  void BeginComputePass();
  void Dispatch(u32 x, u32 y, u32 z);
  void EndComputePass();
};

class DeviceQueue final
{
public:
  RenderCommandBuffer  CreateRenderCommandBuffer();
  ComputeCommandBuffer CreateComputeCommandBuffer();

  void SubmitCommandBuffer(RenderCommandBuffer commandBuffer);
  void SubmitCommandBuffer(ComputeCommandBuffer commandBuffer);
};

} // namespace kronk
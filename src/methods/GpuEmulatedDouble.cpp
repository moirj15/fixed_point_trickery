#include "GpuEmulatedDouble.hpp"

namespace methods
{
GpuEmulatedDoubleMethod::GpuEmulatedDoubleMethod(
  ID3D11Device3 *device,
  ShaderWatcher &shaderWatcher)
{
}

void GpuEmulatedDoubleMethod::SetScene(const Scene &scene)
{
}

void GpuEmulatedDoubleMethod::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos)
{
}

void GpuEmulatedDoubleMethod::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
}

} // namespace methods

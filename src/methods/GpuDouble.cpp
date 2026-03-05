#include "GpuDouble.hpp"

namespace methods
{

GpuDoubleMethod::GpuDoubleMethod(ID3D11Device3 *device, ShaderWatcher &shaderWatcher)
{
}

void GpuDoubleMethod::SetScene(const Scene &scene)
{
}

void GpuDoubleMethod::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos)
{
}

void GpuDoubleMethod::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
}

} // namespace methods
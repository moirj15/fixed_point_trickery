#include "CpuDouble.hpp"

namespace methods
{
CpuDoubleMethod::CpuDoubleMethod(ID3D11Device3 *device, ShaderWatcher &shaderWatcher)
{
}

void CpuDoubleMethod::SetScene(const Scene &scene)
{
}

void CpuDoubleMethod::Update(
  ID3D11DeviceContext3 *ctx,
  const glm::dmat4     &cameraProjection,
  const glm::dvec3     &modelPos)
{
}

void CpuDoubleMethod::Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher)
{
}

} // namespace methods


#pragma once

#include "../dx.hpp"
#include "../shaderWatcher.hpp"

#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

struct Scene;
struct RenderContext;

namespace methods
{

class Parallax final
{
public:
  explicit Parallax(ID3D11Device3 *device, ShaderWatcher &shaderWatcher);

  void SetScene(const Scene &scene);

  void
  Update(ID3D11DeviceContext3 *ctx, const glm::dmat4 &cameraProjection, const glm::dvec3 &modelPos);

  void Draw(dx::RenderContext &renderContext, ShaderWatcher &shaderWatcher);
};

} // namespace methods

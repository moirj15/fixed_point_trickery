#pragma once

struct Scene;
struct RenderContext;

namespace methods
{

class CpuDoubleMethod final
{
public:
  explicit CpuDoubleMethod(const Scene &scene, const RenderContext &context);

  void Draw();
};

} // namespace methods

#pragma once

struct Scene;
struct RenderContext;

class CpuDoubleMethod final
{
public:
  explicit CpuDoubleMethod(const Scene &scene, const RenderContext &context);

  void Draw();
};

#pragma once

#include "../dx.hpp"

namespace kronk::tests
{

class D3D11Fixture
{
  static constexpr u32 WIDTH  = 512;
  static constexpr u32 HEIGHT = 512;

protected:
  Window        mWindow;
  RenderContext mContext;

public:
  D3D11Fixture() : mWindow{CreateWin(WIDTH, HEIGHT, "test win")}, mContext{InitContext(mWindow)}
  {
  }

  ~D3D11Fixture()
  {
    DestroyWin(mWindow);
  }
};

} // namespace kronk::tests
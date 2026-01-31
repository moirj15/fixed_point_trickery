
#include "../dx.hpp"
#include "../shaderWatcher.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>

class ShaderWatcherFixture
{
protected:
  std::unique_ptr<ShaderWatcher> watcher{};
  ComPtr<ID3D11Device3>          mDevice;

public:
  ShaderWatcherFixture()
  {
    u32 createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
    createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
    D3D_FEATURE_LEVEL desiredLevel[] = {D3D_FEATURE_LEVEL_11_1};
    D3D_FEATURE_LEVEL featureLevel;
    // TODO: get latest dx11
    ComPtr<ID3D11Device>        baseDevice;
    ComPtr<ID3D11DeviceContext> baseContext;
    dx::ThrowIfFailed(D3D11CreateDevice(
      nullptr,
      D3D_DRIVER_TYPE_HARDWARE,
      nullptr,
      createDeviceFlags,
      desiredLevel,
      1,
      D3D11_SDK_VERSION,
      baseDevice.GetAddressOf(),
      &featureLevel,
      baseContext.GetAddressOf()));
    assert(featureLevel == D3D_FEATURE_LEVEL_11_1);

    baseDevice->QueryInterface(__uuidof(ID3D11Device3), &mDevice);

    watcher = std::make_unique<ShaderWatcher>(mDevice.Get());
  }
};

TEST_CASE_METHOD(ShaderWatcherFixture, "Compile fresh vert and pixel shader")
{
  auto p = std::filesystem::current_path();
  REQUIRE_NOTHROW((void)watcher->RegisterShader(
    "src/tests/testShaders/vert.hlsl",
    "src/tests/testShaders/pixel.hlsl"));
}

TEST_CASE_METHOD(ShaderWatcherFixture, "Compile fresh compute shader")
{
  REQUIRE_NOTHROW((void)watcher->RegisterShader("src/tests/testShaders/compute.hlsl"));
}

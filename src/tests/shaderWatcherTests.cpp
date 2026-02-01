
#include "../dx.hpp"
#include "../shaderWatcher.hpp"

#include <catch2/catch_test_macros.hpp>
#include <memory>

class ShaderWatcherFixture
{
protected:
  std::unique_ptr<ShaderWatcher> watcher{};
  ComPtr<ID3D11Device3>          mDevice;

  const std::string VERTEX_PATH{"src/tests/testShaders/vert.hlsl"};
  const std::string PIXEL_PATH{"src/tests/testShaders/pixel.hlsl"};
  const std::string COMPUTE_PATH{"src/tests/testShaders/compute.hlsl"};

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
  REQUIRE_NOTHROW((void)watcher->RegisterShader(VERTEX_PATH, PIXEL_PATH));
}

TEST_CASE_METHOD(ShaderWatcherFixture, "Compile fresh compute shader")
{
  REQUIRE_NOTHROW((void)watcher->RegisterShader(COMPUTE_PATH));
}

TEST_CASE_METHOD(ShaderWatcherFixture, "Recompile after vert shader file modified")
{
  const RenderProgramHandle h = watcher->RegisterShader(VERTEX_PATH, PIXEL_PATH);

  RenderProgram rp = watcher->GetRenderProgram(h);
  std::filesystem::last_write_time(
    std::filesystem::path{VERTEX_PATH},
    std::chrono::file_clock::now());
  RenderProgram rp2 = watcher->GetRenderProgram(h);
  REQUIRE(rp.vertexShader != rp2.vertexShader);
  REQUIRE(rp.pixelShader == rp2.pixelShader);
}

TEST_CASE_METHOD(ShaderWatcherFixture, "Recompile after pixel shader file modified")
{
  const RenderProgramHandle h = watcher->RegisterShader(VERTEX_PATH, PIXEL_PATH);

  RenderProgram rp = watcher->GetRenderProgram(h);
  std::filesystem::last_write_time(
    std::filesystem::path{PIXEL_PATH},
    std::chrono::file_clock::now());
  RenderProgram rp2 = watcher->GetRenderProgram(h);
  REQUIRE(rp.pixelShader != rp2.pixelShader);
  REQUIRE(rp.vertexShader == rp2.vertexShader);
}

TEST_CASE_METHOD(ShaderWatcherFixture, "Recompile after compute shader file modified")
{
  const ComputeProgramHandle h = watcher->RegisterShader(COMPUTE_PATH);

  ID3D11ComputeShader *compute = watcher->GetComputeProgram(h);
  std::filesystem::last_write_time(
    std::filesystem::path{COMPUTE_PATH},
    std::chrono::file_clock::now());
  ID3D11ComputeShader *compute2 = watcher->GetComputeProgram(h);
  REQUIRE(compute != compute2);
}

TEST_CASE_METHOD(ShaderWatcherFixture, "No recompilation after accessing same render program")
{
  const RenderProgramHandle h   = watcher->RegisterShader(VERTEX_PATH, PIXEL_PATH);
  const RenderProgram       rp  = watcher->GetRenderProgram(h);
  const RenderProgram       rp2 = watcher->GetRenderProgram(h);
  REQUIRE(rp.vertexShader == rp2.vertexShader);
  REQUIRE(rp.pixelShader == rp2.pixelShader);
}

TEST_CASE_METHOD(ShaderWatcherFixture, "No recompilation after accessing same compute shader")
{
  const ComputeProgramHandle h        = watcher->RegisterShader(COMPUTE_PATH);
  ID3D11ComputeShader       *compute  = watcher->GetComputeProgram(h);
  ID3D11ComputeShader       *compute2 = watcher->GetComputeProgram(h);
  REQUIRE(compute == compute2);
}

TEST_CASE_METHOD(ShaderWatcherFixture, "Throw when shader type missmatch")
{
  SECTION("vertex - pixel missmatch")
  {
    REQUIRE_THROWS((void)watcher->RegisterShader(PIXEL_PATH, VERTEX_PATH));
  }

  SECTION("compute - vertex missmatch")
  {
    REQUIRE_THROWS((void)watcher->RegisterShader(COMPUTE_PATH, PIXEL_PATH));
  }

  SECTION("compute - pixel missmatch")
  {
    REQUIRE_THROWS((void)watcher->RegisterShader(VERTEX_PATH, COMPUTE_PATH));
  }

  SECTION("vertex - compute missmatch")
  {
    REQUIRE_THROWS((void)watcher->RegisterShader(VERTEX_PATH));
  }

  SECTION("pixel - compute missmatch")
  {
    REQUIRE_THROWS((void)watcher->RegisterShader(PIXEL_PATH));
  }
}

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <Windows.h>
#include <cassert>
#include <d3d11_3.h>
#include <d3d11shader.h>
#include <directxtk/BufferHelpers.h>
#include <iostream>
#include <wrl/client.h>

using u64 = uint64_t;
using u32 = uint32_t;
using u16 = uint16_t;
using u8  = uint8_t;

using i64 = int64_t;
using i32 = int32_t;
using i16 = int16_t;
using i8  = int8_t;

using f32 = float;
using f64 = double;

constexpr u32 WIDTH  = 1920;
constexpr u32 HEIGHT = 1080;

using Microsoft::WRL::ComPtr;
#include <exception>

namespace DX
{
// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
  com_exception(HRESULT hr) :
      mResult{hr}, mMessage{std::format("Failure with HRESULT of {:#8X}", mResult)}
  {
  }

  const char *what() const noexcept override
  {
    return mMessage.c_str();
  }

private:
  HRESULT     mResult;
  std::string mMessage;
};

// Helper utility converts D3D API failures into exceptions.
inline void ThrowIfFailed(HRESULT hr)
{
  if (FAILED(hr)) {
    throw com_exception(hr);
  }
}
} // namespace DX

constexpr void Check(HRESULT result)
{
  if (FAILED(result)) {
    throw std::runtime_error(std::format("Got error code {}", result));
  }
}
struct RenderContext
{
  ComPtr<ID3D11Device3>         m_device;
  ComPtr<ID3D11DeviceContext3>  m_context;
  ComPtr<ID3D11RasterizerState> m_rasterizer_state;

  ComPtr<IDXGISwapChain>         m_swapchain;
  ComPtr<ID3D11Texture2D>        m_backbuffer;
  ComPtr<ID3D11RenderTargetView> m_backbuffer_render_target_view;
  ComPtr<ID3D11Texture2D>        m_depth_stencil_buffer;
  ComPtr<ID3D11DepthStencilView> m_depth_stencil_view;

  ID3D11Device3 *Device() const
  {
    return m_device.Get();
  }
  ID3D11DeviceContext3 *DeviceContext() const
  {
    return m_context.Get();
  }
};

RenderContext InitContext(HWND window_handle, u32 window_width, u32 window_height)
{
  RenderContext context{};
  u32           createDeviceFlags = 0;
#if defined(DEBUG) || defined(_DEBUG)
  createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
#endif
  D3D_FEATURE_LEVEL desiredLevel[] = {D3D_FEATURE_LEVEL_11_1};
  D3D_FEATURE_LEVEL featureLevel;
  // TODO: get latest dx11
  ID3D11Device        *baseDevice;
  ID3D11DeviceContext *baseContext;
  Check(D3D11CreateDevice(
    nullptr,
    D3D_DRIVER_TYPE_HARDWARE,
    nullptr,
    createDeviceFlags,
    desiredLevel,
    1,
    D3D11_SDK_VERSION,
    &baseDevice,
    &featureLevel,
    &baseContext));
  assert(featureLevel == D3D_FEATURE_LEVEL_11_1);

  baseDevice->QueryInterface(__uuidof(ID3D11Device3), &context.m_device);
  baseContext->QueryInterface(__uuidof(ID3D11DeviceContext3), &context.m_context);

  // TODO: temporary rasterizer state, should create a manager for this, maybe create a handle type
  // for this?
  // TODO: maybe an internal handle just for tracking this internaly?
  D3D11_RASTERIZER_DESC rasterizerDesc = {
    .FillMode              = D3D11_FILL_SOLID,
    .CullMode              = D3D11_CULL_NONE,
    .FrontCounterClockwise = true,
  };

  context.m_device->CreateRasterizerState(&rasterizerDesc, &context.m_rasterizer_state);

  DXGI_SWAP_CHAIN_DESC swapChainDesc = {
    .BufferDesc =
      {
        .Width  = window_width,
        .Height = window_height,
        .RefreshRate =
          {
            .Numerator   = 60,
            .Denominator = 1,
          },
        .Format           = DXGI_FORMAT_R8G8B8A8_UNORM,
        .ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED,
        .Scaling          = DXGI_MODE_SCALING_UNSPECIFIED,
      },
    // Multi sampling would be initialized here
    .SampleDesc =
      {
        .Count   = 1,
        .Quality = 0,
      },
    .BufferUsage  = DXGI_USAGE_RENDER_TARGET_OUTPUT,
    .BufferCount  = 1,
    .OutputWindow = window_handle,
    .Windowed     = true,
    .SwapEffect   = DXGI_SWAP_EFFECT_DISCARD,
    .Flags        = 0,
  };
  // clang-format on

  Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
  DX::ThrowIfFailed(context.m_device->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice));
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  DX::ThrowIfFailed(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &adapter));
  Microsoft::WRL::ComPtr<IDXGIFactory> factory;
  DX::ThrowIfFailed(adapter->GetParent(__uuidof(IDXGIFactory), &factory));

  DX::ThrowIfFailed(
    factory->CreateSwapChain(context.m_device.Get(), &swapChainDesc, &context.m_swapchain));

  context.m_swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), &context.m_backbuffer);
  context.m_device->CreateRenderTargetView(
    context.m_backbuffer.Get(),
    0,
    &context.m_backbuffer_render_target_view);

  D3D11_TEXTURE2D_DESC depthStencilDesc = {
    .Width     = window_width,
    .Height    = window_height,
    .MipLevels = 1,
    .ArraySize = 1,
    .Format    = DXGI_FORMAT_D24_UNORM_S8_UINT,
    // Multi sampling here
    .SampleDesc =
      {
        .Count   = 1,
        .Quality = 0,
      },
    .Usage          = D3D11_USAGE_DEFAULT,
    .BindFlags      = D3D11_BIND_DEPTH_STENCIL,
    .CPUAccessFlags = 0,
    .MiscFlags      = 0,
  };

  DX::ThrowIfFailed(
    context.m_device->CreateTexture2D(&depthStencilDesc, 0, &context.m_depth_stencil_buffer));
  DX::ThrowIfFailed(context.m_device->CreateDepthStencilView(
    context.m_depth_stencil_buffer.Get(),
    0,
    &context.m_depth_stencil_view));
  context.m_context->OMSetRenderTargets(
    1,
    context.m_backbuffer_render_target_view.GetAddressOf(),
    context.m_depth_stencil_view.Get());

  return context;
}

int main(int argc, char **argv)
{
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  SDL_Window *window = SDL_CreateWindow(
    "D3D Viewer",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    WIDTH,
    HEIGHT,
    SDL_WINDOW_SHOWN);

  assert(window != nullptr);

  SDL_SysWMinfo win_info;
  SDL_VERSION(&win_info.version);
  SDL_GetWindowWMInfo(window, &win_info);
  HWND hwnd = win_info.info.win.window;

  RenderContext ctx          = InitContext(hwnd, WIDTH, HEIGHT);
  f32           clearColor[] = {0.5, 0.5, 0.5, 1.0};
  ctx.m_context->ClearRenderTargetView(ctx.m_backbuffer_render_target_view.Get(), clearColor);
  ctx.m_swapchain->Present(1, 0);

  SDL_Event e;
  bool      running = true;
  while (running) {
    while (SDL_PollEvent(&e) > 0) {
      if (e.type == SDL_QUIT) {
        running = false;
      }
    }
  }
  return 0;
}

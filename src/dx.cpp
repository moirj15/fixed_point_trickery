#include "dx.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
namespace dx
{

Window CreateWin(u32 width, u32 height, const char *title)
{
  assert(SDL_Init(SDL_INIT_VIDEO) == 0);
  SDL_Window *window = SDL_CreateWindow(
    "D3D Viewer",
    SDL_WINDOWPOS_UNDEFINED,
    SDL_WINDOWPOS_UNDEFINED,
    width,
    height,
    SDL_WINDOW_SHOWN);

  assert(window != nullptr);

  SDL_SysWMinfo win_info;
  SDL_VERSION(&win_info.version);
  SDL_GetWindowWMInfo(window, &win_info);
  HWND hwnd = win_info.info.win.window;

  return {
    .win    = window,
    .width  = width,
    .height = height,
    .hwnd   = hwnd,
  };
}

void DestroyWin(Window &win)
{
  SDL_DestroyWindow(win.win);
  win = {};
}

RenderContext InitContext(const Window &window)
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

  baseDevice->QueryInterface(__uuidof(ID3D11Device3), &context.device);
  baseContext->QueryInterface(__uuidof(ID3D11DeviceContext3), &context.context);

  // TODO: temporary rasterizer state, should create a manager for this, maybe create a handle type
  // for this?
  // TODO: maybe an internal handle just for tracking this internally?
  D3D11_RASTERIZER_DESC rasterizerDesc = {
    .FillMode              = D3D11_FILL_SOLID,
    .CullMode              = D3D11_CULL_NONE,
    .FrontCounterClockwise = true,
  };

  context.device->CreateRasterizerState(&rasterizerDesc, &context.rasterizerState);

  DXGI_SWAP_CHAIN_DESC swapChainDesc = {
    .BufferDesc =
      {
        .Width  = window.width,
        .Height = window.height,
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
    .OutputWindow = window.hwnd,
    .Windowed     = true,
    .SwapEffect   = DXGI_SWAP_EFFECT_DISCARD,
    .Flags        = 0,
  };
  // clang-format on

  Microsoft::WRL::ComPtr<IDXGIDevice> dxgiDevice;
  dx::ThrowIfFailed(context.device->QueryInterface(__uuidof(IDXGIDevice), &dxgiDevice));
  Microsoft::WRL::ComPtr<IDXGIAdapter> adapter;
  dx::ThrowIfFailed(dxgiDevice->GetParent(__uuidof(IDXGIAdapter), &adapter));
  Microsoft::WRL::ComPtr<IDXGIFactory> factory;
  dx::ThrowIfFailed(adapter->GetParent(__uuidof(IDXGIFactory), &factory));

  dx::ThrowIfFailed(
    factory->CreateSwapChain(context.device.Get(), &swapChainDesc, &context.swapchain));

  context.swapchain->GetBuffer(0, __uuidof(ID3D11Texture2D), &context.backbuffer);
  context.device->CreateRenderTargetView(context.backbuffer.Get(), 0, &context.backbufferRTV);

  D3D11_TEXTURE2D_DESC depthStencilDesc = {
    .Width     = window.width,
    .Height    = window.height,
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

  dx::ThrowIfFailed(
    context.device->CreateTexture2D(&depthStencilDesc, 0, &context.depthStencilBuffer));
  dx::ThrowIfFailed(context.device->CreateDepthStencilView(
    context.depthStencilBuffer.Get(),
    0,
    &context.depthStencilView));
  context.context->OMSetRenderTargets(
    1,
    context.backbufferRTV.GetAddressOf(),
    context.depthStencilView.Get());

  D3D11_VIEWPORT viewport = {
    .TopLeftX = 0.0f,
    .TopLeftY = 0.0f,
    .Width    = static_cast<f32>(window.width),
    .Height   = static_cast<f32>(window.height),
    .MinDepth = 0.0,
    .MaxDepth = 1.0,
  };

  context.context->RSSetViewports(1, &viewport);

  return context;
}

RenderPipeline
CreatePipeline(std::string_view vertexPath, std::string_view pixelPath, ID3D11Device3 *device)
{

  RenderPipeline pipeline{};

  ComPtr<ID3D10Blob> vertexByteCode;
  ComPtr<ID3D10Blob> vertexError;
  ComPtr<ID3D10Blob> pixelByteCode;
  ComPtr<ID3D10Blob> pixelError;

  auto s = std::filesystem::current_path() / vertexPath;
  auto modifiedVertexPath =
    std::filesystem::canonical(std::filesystem::current_path() / vertexPath);
  auto modifiedPixelPath = std::filesystem::canonical(std::filesystem::current_path() / pixelPath);

#if 0
   dx::ThrowIfFailed(D3DReadFileToBlob(
     reinterpret_cast<LPCWSTR>(modifiedVertexPath.c_str()),
     vertexByteCode.GetAddressOf()));
   dx::ThrowIfFailed(D3DReadFileToBlob(
     reinterpret_cast<LPCWSTR>(modifiedPixelPath.c_str()),
     pixelByteCode.GetAddressOf()));
#endif

  dx::ThrowIfFailed(D3DCompileFromFile(
    reinterpret_cast<LPCWSTR>(modifiedVertexPath.c_str()),
    nullptr,
    nullptr,
    "VSMain",
    "vs_5_0",
    0,
    0,
    vertexByteCode.GetAddressOf(),
    vertexError.GetAddressOf()));

  dx::ThrowIfFailed(D3DCompileFromFile(
    reinterpret_cast<LPCWSTR>(modifiedPixelPath.c_str()),
    nullptr,
    nullptr,
    "PSMain",
    "ps_5_0",
    0,
    0,
    pixelByteCode.GetAddressOf(),
    pixelError.GetAddressOf()));

  dx::ThrowIfFailed(device->CreateVertexShader(
    vertexByteCode->GetBufferPointer(),
    vertexByteCode->GetBufferSize(),
    nullptr,
    pipeline.vertexShader.GetAddressOf()));

  dx::ThrowIfFailed(device->CreatePixelShader(
    pixelByteCode->GetBufferPointer(),
    pixelByteCode->GetBufferSize(),
    nullptr,
    pipeline.pixelShader.GetAddressOf()));

  CD3D11_RASTERIZER_DESC rasterizerDesc{CD3D11_DEFAULT{}};
  rasterizerDesc.FrontCounterClockwise = true;

  device->CreateRasterizerState(&rasterizerDesc, pipeline.rasterizerState.GetAddressOf());

  return pipeline;
}
} // namespace dx
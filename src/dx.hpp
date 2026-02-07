#pragma once

#include "utils.hpp"

#include <d3d11_3.h>
#include <d3d11shader.h>
#include <d3dcompiler.h>
#include <directxtk/BufferHelpers.h>
#include <exception>
#include <filesystem>
#include <format>
#include <span>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;
struct SDL_Window;

namespace dx
{

template<typename T>
constexpr u32 GpuSizeof()
{
  return static_cast<u32>(sizeof(T));
}

// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
  com_exception(HRESULT hr) :
      mResult{hr},
      mMessage{std::format("Failure with HRESULT of {:#8X}", mResult)}
  {
  }

  constexpr const char *what() const noexcept override
  {
    return mMessage.c_str();
  }

private:
  HRESULT     mResult;
  std::string mMessage;
};

// Helper utility converts D3D API failures into exceptions.
constexpr void ThrowIfFailed(HRESULT hr)
{
  if (FAILED(hr))
  {
    throw com_exception(hr);
  }
}

constexpr void Check(HRESULT result)
{
  if (FAILED(result))
  {
    throw std::runtime_error(std::format("Got error code {}", result));
  }
}

struct Window
{
  SDL_Window *win{};
  u32         width{};
  u32         height{};
  HWND        hwnd{};
};

Window CreateWin(u32 width, u32 height, const char *title);
void   DestroyWin(Window &win);

struct RenderContext
{
  ComPtr<ID3D11Device3>         device;
  ComPtr<ID3D11DeviceContext3>  context;
  ComPtr<ID3D11RasterizerState> rasterizerState;

  ComPtr<IDXGISwapChain>         swapchain;
  ComPtr<ID3D11Texture2D>        backbuffer;
  ComPtr<ID3D11RenderTargetView> backbufferRTV;
  ComPtr<ID3D11Texture2D>        depthStencilBuffer;
  ComPtr<ID3D11DepthStencilView> depthStencilView;

  ID3D11Device3 *Device() const
  {
    return device.Get();
  }
  ID3D11DeviceContext3 *DeviceContext() const
  {
    return context.Get();
  }
};

RenderContext InitContext(const Window &window);

struct RenderPipeline
{
  ComPtr<ID3D11VertexShader>    vertexShader;
  ComPtr<ID3D11PixelShader>     pixelShader;
  ComPtr<ID3D11RasterizerState> rasterizerState;
};

RenderPipeline
CreatePipeline(std::string_view vertexPath, std::string_view pixelPath, ID3D11Device3 *device);

struct VertexBuffer
{
  ComPtr<ID3D11Buffer>             buf;
  ComPtr<ID3D11ShaderResourceView> view;
};

template<typename T>
VertexBuffer CreateVertexBuffer(ID3D11Device3 *device, u32 count, std::span<const T> data)
{
  D3D11_BUFFER_DESC desc = {
    .ByteWidth           = count * GpuSizeof<T>(),
    .Usage               = D3D11_USAGE_DYNAMIC,
    .BindFlags           = D3D11_BIND_SHADER_RESOURCE,
    .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
    .StructureByteStride = GpuSizeof<T>(),
  };

  ID3D11Buffer *vertexBuffer{};
  if (data.empty())
  {
    dx::ThrowIfFailed(device->CreateBuffer(&desc, nullptr, &vertexBuffer));
  }
  else
  {
    D3D11_SUBRESOURCE_DATA d{};
    d.pSysMem = data.data();
    dx::ThrowIfFailed(device->CreateBuffer(&desc, &d, &vertexBuffer));
  }

  ID3D11ShaderResourceView       *view{};
  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
  viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.ElementWidth = count;

  dx::ThrowIfFailed(device->CreateShaderResourceView(vertexBuffer, &viewDesc, &view));

  return {
    .buf  = vertexBuffer,
    .view = view,
  };
}

template<typename T>
ComPtr<ID3D11Buffer> CreateIndexBuffer(ID3D11Device3 *device, u32 size, std::span<const T> data)
{
  D3D11_BUFFER_DESC desc = {
    .ByteWidth           = size * GpuSizeof<T>(),
    .Usage               = D3D11_USAGE_DYNAMIC,
    .BindFlags           = D3D11_BIND_INDEX_BUFFER,
    .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags           = {},
    .StructureByteStride = {},
  };

  ID3D11Buffer *indexBuffer{};
  if (data.empty())
  {
    device->CreateBuffer(&desc, nullptr, &indexBuffer);
  }
  else
  {
    D3D11_SUBRESOURCE_DATA d{};
    d.pSysMem = data.data();
    device->CreateBuffer(&desc, &d, &indexBuffer);
  }

  return indexBuffer;
}

template<typename T>
ComPtr<ID3D11Buffer> CreateConstantBuffer(ID3D11Device3 *device, const T *data)
{
  constexpr u32 size = GpuSizeof<T>();

  D3D11_BUFFER_DESC desc = {
    .ByteWidth           = size + 128 - (size % 128),
    .Usage               = D3D11_USAGE_DYNAMIC,
    .BindFlags           = D3D11_BIND_CONSTANT_BUFFER,
    .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags           = {},
    .StructureByteStride = {},
  };

  ID3D11Buffer *constantBuffer{};
  if (data == nullptr)
  {
    device->CreateBuffer(&desc, nullptr, &constantBuffer);
  }
  else
  {
    D3D11_SUBRESOURCE_DATA d{};
    d.pSysMem = data;
    device->CreateBuffer(&desc, &d, &constantBuffer);
  }

  return constantBuffer;
}
} // namespace dx

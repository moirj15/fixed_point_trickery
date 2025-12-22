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

namespace kronk::dx
{

template<typename T>
u32 GpuSizeof()
{
  return static_cast<u32>(sizeof(T));
}

// Helper class for COM exceptions
class com_exception : public std::exception
{
public:
  com_exception(HRESULT hr) :
      mResult{hr}, mMessage{std::format("Failure with HRESULT of {:#8X}", mResult)}
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
} // namespace kronk::dx

namespace kronk
{
constexpr void Check(HRESULT result)
{
  if (FAILED(result))
  {
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

RenderContext InitContext(HWND window_handle, u32 window_width, u32 window_height);

struct RenderPipeline
{
  ComPtr<ID3D11VertexShader>    vertexShader;
  ComPtr<ID3D11PixelShader>     pixelShader;
  ComPtr<ID3D11RasterizerState> rasterizerState;
};

RenderPipeline
CreatePipeline(std::string_view vertexPath, std::string_view pixelPath, ID3D11Device3 *device);

template<typename T>
ComPtr<ID3D11Buffer>
CreateVertexBuffer(ID3D11Device3 *device, u32 size, u32 stride, std::span<const T> data)
{
  D3D11_BUFFER_DESC desc = {
    .ByteWidth           = size,
    .Usage               = D3D11_USAGE_DYNAMIC,
    .BindFlags           = D3D11_BIND_SHADER_RESOURCE,
    .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
    .MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
    .StructureByteStride = stride,
  };

  ID3D11Buffer *vertexBuffer{};
  if (data.empty())
  {
    device->CreateBuffer(&desc, nullptr, &vertexBuffer);
  }
  else
  {
    D3D11_SUBRESOURCE_DATA d{};
    d.pSysMem = data.data();
    device->CreateBuffer(&desc, &d, &vertexBuffer);
  }

  return vertexBuffer;
}

template<typename T>
ComPtr<ID3D11Buffer> CreateIndexBuffer(ID3D11Device3 *device, u32 size, std::span<const T> data)
{
  D3D11_BUFFER_DESC desc = {
    .ByteWidth           = size,
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
  D3D11_BUFFER_DESC desc = {
    .ByteWidth           = sizeof(T),
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
} // namespace kronk

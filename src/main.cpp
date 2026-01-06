#include "Handle.hpp"
#include "dx.hpp"
#include "utils.hpp"

#include <SDL2/SDL.h>
#include <SDL2/SDL_syswm.h>
#include <Windows.h>
#include <array>
#include <cassert>
#include <glm/glm.hpp>

constexpr u32 WIDTH  = 1920;
constexpr u32 HEIGHT = 1080;

int main(int argc, char **argv)
{
  const kronk::Window window = kronk::CreateWin(WIDTH, HEIGHT, "win");

  kronk::RenderContext ctx = kronk::InitContext(window);

  kronk::RenderPipeline colordNormalsPipeline = kronk::CreatePipeline(
    "shaders/colored_normals.hlsl",
    "shaders/colored_normals.hlsl",
    ctx.m_device.Get());

  struct Vertex
  {
    glm::vec3 pos;
    glm::vec3 normal;
  };

  std::array<Vertex, 3> tri = {
    Vertex{glm::vec3{0, 1, 0}, glm::normalize(glm::vec3{0, 1, 0})},
    Vertex{glm::vec3{-1, -1, 0}, glm::normalize(glm::vec3{-1, -1, 0})},
    Vertex{glm::vec3{1, -1, 0}, glm::normalize(glm::vec3{1, -1, 0})},
  };

  std::array<u32, 3> indices = {0, 1, 2};

#if 0
  kronk::BufferHandle vb =
    kronk::CreateVertexBuffer<glm::vec3>(ctx.m_device.Get(), sizeof(tri), sizeof(glm::vec3), tri);
#endif

  D3D11_SHADER_RESOURCE_VIEW_DESC viewDesc{};
  viewDesc.Format              = DXGI_FORMAT_UNKNOWN;
  viewDesc.ViewDimension       = D3D11_SRV_DIMENSION_BUFFER;
  viewDesc.Buffer.ElementWidth = tri.size();

  ComPtr<ID3D11ShaderResourceView> vbView;

#if 0
  kronk::dx::ThrowIfFailed(
    ctx.m_device->CreateShaderResourceView(vb.Get(), &viewDesc, vbView.GetAddressOf()));
#endif

  ComPtr<ID3D11Buffer> ib =
    kronk::CreateIndexBuffer<u32>(ctx.m_device.Get(), sizeof(indices), indices);

  const glm::mat4      mvp = glm::mat4{1.0};
  ComPtr<ID3D11Buffer> cb  = kronk::CreateConstantBuffer<glm::mat4x4>(ctx.m_device.Get(), &mvp);

  f32 clearColor[] = {0.5, 0.5, 0.5, 1.0};
  ctx.m_context->RSSetState(colordNormalsPipeline.rasterizerState.Get());
  D3D11_VIEWPORT viewport = {
    .TopLeftX = 0.0f,
    .TopLeftY = 0.0f,
    .Width    = static_cast<f32>(window.width),
    .Height   = static_cast<f32>(window.height),
    .MinDepth = 0.0,
    .MaxDepth = 1.0,
  };
  ctx.m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx.m_context->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R32_UINT, 0);
  ctx.m_context->RSSetViewports(1, &viewport);
  ctx.m_context->ClearRenderTargetView(ctx.m_backbuffer_render_target_view.Get(), clearColor);
  ctx.m_swapchain->Present(1, 0);
  ctx.m_context->VSSetShader(colordNormalsPipeline.vertexShader.Get(), nullptr, 0);
  ctx.m_context->PSSetShader(colordNormalsPipeline.pixelShader.Get(), nullptr, 0);
  std::array resourceViews = {vbView.Get()};
  ctx.m_context->VSSetShaderResources(0, 1, resourceViews.data());
  ctx.m_context->VSSetConstantBuffers(0, 1, cb.GetAddressOf());
  ctx.m_context->Draw(3, 0);
  ctx.m_swapchain->Present(1, 0);

  // LoadModel();

  SDL_Event e;
  bool      running = true;
  while (running)
  {
    while (SDL_PollEvent(&e) > 0)
    {
      if (e.type == SDL_QUIT)
      {
        running = false;
      }
    }
    ctx.m_context->ClearRenderTargetView(ctx.m_backbuffer_render_target_view.Get(), clearColor);
    ctx.m_context->ClearDepthStencilView(ctx.m_depth_stencil_view.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);
    ctx.m_context->Draw(3, 0);
    ctx.m_swapchain->Present(1, 0);
  }
  return 0;
}

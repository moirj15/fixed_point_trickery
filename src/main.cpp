#include "dx.hpp"
#include "methods/F32.hpp"
#include "modelLoader.hpp"
#include "shaderWatcher.hpp"
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
  const dx::Window window = dx::CreateWin(WIDTH, HEIGHT, "win");

  dx::RenderContext ctx = dx::InitContext(window);

#if 0
  dx::RenderPipeline colordNormalsPipeline = dx::CreatePipeline(
    "shaders/colored_normals.hlsl",
    "shaders/colored_normals.hlsl",
    ctx.device.Get());

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

  dx::VertexBuffer vb =
    dx::CreateVertexBuffer<Vertex>(ctx.device.Get(), sizeof(tri), sizeof(glm::vec3), tri);

  ComPtr<ID3D11Buffer> ib = dx::CreateIndexBuffer<u32>(ctx.device.Get(), sizeof(indices), indices);

  const glm::mat4      mvp = glm::mat4{1.0};
  ComPtr<ID3D11Buffer> cb  = dx::CreateConstantBuffer<glm::mat4x4>(ctx.device.Get(), &mvp);

  f32 clearColor[] = {0.5, 0.5, 0.5, 1.0};
  ctx.context->RSSetState(colordNormalsPipeline.rasterizerState.Get());
  D3D11_VIEWPORT viewport = {
    .TopLeftX = 0.0f,
    .TopLeftY = 0.0f,
    .Width    = static_cast<f32>(window.width),
    .Height   = static_cast<f32>(window.height),
    .MinDepth = 0.0,
    .MaxDepth = 1.0,
  };
  ctx.context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
  ctx.context->IASetIndexBuffer(ib.Get(), DXGI_FORMAT_R32_UINT, 0);
  ctx.context->RSSetViewports(1, &viewport);
  ctx.context->ClearRenderTargetView(ctx.backbufferRTV.Get(), clearColor);
  ctx.swapchain->Present(1, 0);
  ctx.context->VSSetShader(colordNormalsPipeline.vertexShader.Get(), nullptr, 0);
  ctx.context->PSSetShader(colordNormalsPipeline.pixelShader.Get(), nullptr, 0);
  std::array resourceViews = {vb.view.Get()};
  ctx.context->VSSetShaderResources(0, 1, resourceViews.data());
  ctx.context->VSSetConstantBuffers(0, 1, cb.GetAddressOf());
  ctx.context->Draw(3, 0);
  ctx.swapchain->Present(1, 0);
#endif

  // LoadModel();
  f32 clearColor[] = {0.5, 0.5, 0.5, 1.0};

  Model              model = LoadModel("models/suzanne.glb");
  ShaderWatcher      shaderWatcher{ctx.Device()};
  methods::F32Method f32Method{ctx.Device(), shaderWatcher};
  f32Method.Update(ctx.DeviceContext());

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
#if 0
    ctx.context->ClearRenderTargetView(ctx.backbufferRTV.Get(), clearColor);
    ctx.context->ClearDepthStencilView(ctx.depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);
    ctx.context->Draw(3, 0);
#endif
    ctx.context->ClearRenderTargetView(ctx.backbufferRTV.Get(), clearColor);
    ctx.context->ClearDepthStencilView(ctx.depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);
    f32Method.Draw(ctx, shaderWatcher);
    ctx.swapchain->Present(1, 0);
  }
  return 0;
}

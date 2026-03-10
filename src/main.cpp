#include "arcball_camera.h"
#include "dx.hpp"
#include "methods/CpuDouble.hpp"
#include "methods/F32.hpp"
#include "methods/GpuDouble.hpp"
#include "modelLoader.hpp"
#include "shaderWatcher.hpp"
#include "utils.hpp"

#include <SDL2/SDL.h>
#include <array>
#include <cassert>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <imgui.h>
#include <imgui_impl_dx11.h>
#include <imgui_impl_sdl2.h>

constexpr u32 WIDTH  = 1920;
constexpr u32 HEIGHT = 1080;

enum class Method
{
  F32,
  CpuDouble,
  GpuDouble,
};

template<typename T>
constexpr size_t ToIndex(T t)
{
  return static_cast<size_t>(t);
}

enum class SceneModel
{
  Suzanne,
  AppoloLunarModule,
  ISS,
  Jwst,
  Count,
};

const std::array paths = {
  "models/suzanne.glb",
  "models/apollo_lunar_module.glb",
  "models/ISS.glb",
  "models/jwst.glb",
};

int main(int argc, char **argv)
{
  const dx::Window window = dx::CreateWin(WIDTH, HEIGHT, "win");

  dx::RenderContext ctx = dx::InitContext(window);

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::StyleColorsClassic();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  ImGui_ImplSDL2_InitForD3D(window.win);
  ImGui_ImplDX11_Init(ctx.Device(), ctx.DeviceContext());

  f32 clearColor[] = {0.5, 0.5, 0.5, 1.0};

  ShaderWatcher shaderWatcher{ctx.Device()};

  glm::dmat4    projection = glm::infinitePerspective(90.0, (double)WIDTH / (double)HEIGHT, 0.001);
  ArcballCamera arcballCamera{
    glm::dvec3{0.0, 0.0, 5.0},
    glm::dvec3{0.0, 0.0, -1.0},
    glm::dvec3{0.0, 1.0, 0.0}};

  SDL_Event e;
  bool      running       = true;
  bool      firstBtnPress = false;
  bool      btnReleased   = true;

  i32 lastMouseX{}, lastMouseY{};
  i32 currMouseX{}, currMouseY{};

  auto ToNDC = [](i32 x, i32 y) -> glm::vec2 {
    return {
      ((f32)x / WIDTH) * 2.0 - 1.0,
      -(((f32)y / HEIGHT) * 2.0 - 1.0),
    };
  };

  u32         currTime{}, lastTime{};
  Method      method{};
  SceneModel  currentModel{};
  std::string currentModelPath;
  Model       model = LoadModel("models/suzanne.glb");
  Scene       scene{{model}};

  methods::F32Method       f32Method{ctx.Device(), shaderWatcher};
  methods::CpuDoubleMethod cpuDoubleMethod{ctx.Device(), shaderWatcher};
  methods::GpuDoubleMethod gpuDoubleMethod{ctx.Device(), shaderWatcher};
  f32Method.SetScene(scene);
  cpuDoubleMethod.SetScene(scene);
  gpuDoubleMethod.SetScene(scene);

  glm::vec3  modelPos{0.0, 0.0, 0.0};
  glm::dmat4 modelTranslation = glm::identity<glm::dmat4>();

  while (running)
  {
    lastTime        = currTime;
    currTime        = SDL_GetPerformanceCounter();
    const f64 delta = (((f64)currTime - (f64)lastTime) / SDL_GetPerformanceFrequency());
    while (SDL_PollEvent(&e) > 0)
    {
      ImGui_ImplSDL2_ProcessEvent(&e);
      if (e.type == SDL_QUIT)
      {
        running = false;
      }
    }

    ImGui_ImplDX11_NewFrame();
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();
    ImGui::Begin("hi", nullptr, ImGuiWindowFlags_NoResize | ImGuiWindowFlags_AlwaysAutoResize);

    if (ImGui::RadioButton("F32 Method", method == Method::F32))
    {
      method = Method::F32;
    }
    else if (ImGui::RadioButton("CPU Double Method", method == Method::CpuDouble))
    {
      method = Method::CpuDouble;
    }
    else if (ImGui::RadioButton("GPU Double Method", method == Method::GpuDouble))
    {
      method = Method::GpuDouble;
    }

    ImGui::Separator();

    for (u32 i = 0; i < ToIndex(SceneModel::Count); i++)
    {
      SceneModel sceneModel = static_cast<SceneModel>(i);
      if (ImGui::RadioButton(paths[i], currentModel == sceneModel))
      {
        currentModel     = sceneModel;
        currentModelPath = paths[i];

        model = LoadModel(currentModelPath);
        scene = {{model}};
        f32Method.SetScene(scene);
        cpuDoubleMethod.SetScene(scene);
        gpuDoubleMethod.SetScene(scene);
      }
    }

    ImGui::Separator();
    ImGui::InputFloat3("model position", glm::value_ptr(modelPos));

    ImGui::Text(
      "Camera distance from Model: %lf meters",
      glm::length(glm::dvec3{arcballCamera.eye()} - glm::dvec3{modelPos}));
    ImGui::Text("Model distance from Origin: %lf meters", glm::length(glm::dvec3{modelPos}));

    SDL_PumpEvents();
    i32       x{}, y{};
    const u32 btn = SDL_GetMouseState(&x, &y);
    if ((btn & SDL_BUTTON_LMASK) != 0 && !io.WantCaptureMouse)
    {
      if (firstBtnPress)
      {
        firstBtnPress = false;
        lastMouseX    = x;
        lastMouseY    = y;
        currMouseX    = x;
        currMouseY    = y;
      }
      else
      {
        lastMouseX = currMouseX;
        lastMouseY = currMouseY;
        currMouseX = x;
        currMouseY = y;
      }
      btnReleased = false;
      currMouseX  = x;
      currMouseY  = y;
    }
    else
    {
      firstBtnPress = true;
      lastMouseX    = x;
      lastMouseY    = y;
      currMouseX    = x;
      currMouseY    = y;
    }
    const u8 *keyState = SDL_GetKeyboardState(nullptr);
    if (!io.WantCaptureKeyboard)
    {
      double boost = 1.0;
      if (keyState[SDL_SCANCODE_LSHIFT])
      {
        boost = 10.0;
      }
      if (keyState[SDL_SCANCODE_W])
      {
        arcballCamera.zoom(boost * delta);
      }
      else if (keyState[SDL_SCANCODE_S])
      {
        arcballCamera.zoom(-boost * delta);
      }
    }
    arcballCamera.rotate(ToNDC(lastMouseX, lastMouseY), ToNDC(currMouseX, currMouseY));

    ctx.context->ClearRenderTargetView(ctx.backbufferRTV.Get(), clearColor);
    ctx.context->ClearDepthStencilView(ctx.depthStencilView.Get(), D3D11_CLEAR_DEPTH, 1.0, 0);

    switch (method)
    {
    case Method::F32:
      f32Method.Update(
        ctx.DeviceContext(),
        projection * glm::dmat4{arcballCamera.transform()},
        glm::dvec3{modelPos});
      f32Method.Draw(ctx, shaderWatcher);
      break;
    case Method::CpuDouble:
      cpuDoubleMethod.Update(
        ctx.DeviceContext(),
        projection * glm::dmat4{arcballCamera.transform()},
        glm::dvec3{modelPos});
      cpuDoubleMethod.Draw(ctx, shaderWatcher);
      break;
    case Method::GpuDouble:
      gpuDoubleMethod.Update(
        ctx.DeviceContext(),
        projection * glm::dmat4{arcballCamera.transform()},
        glm::dvec3{modelPos});
      gpuDoubleMethod.Draw(ctx, shaderWatcher);
      break;
    default:
      assert(0);
    }

    ImGui::End();

    ImGui::Render();
    // ctx.context->OMSetRenderTargets(1, &)
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ctx.swapchain->Present(1, 0);
  }

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyWindow(window.win);
  SDL_Quit();

  return 0;
}

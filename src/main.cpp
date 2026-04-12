#include "arcball_camera.h"
#include "dx.hpp"
#include "methods/CpuDouble.hpp"
#include "methods/F32.hpp"
#include "methods/GpuDouble.hpp"
#include "methods/GpuEmulatedDouble.hpp"
#include "methods/Parallax.hpp"
#include "modelLoader.hpp"
#include "shaderWatcher.hpp"
#include "utils.hpp"

#include <SDL2/SDL.h>
#include <array>
#include <cassert>
#include <chrono>
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
#if 0
  GpuDouble,
#endif
  GpuEmulatedDouble,
  Parallax,
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

const std::array distances = {
  1e0 + 5.0,
  1e1 + 5.0,
  1e2 + 5.0,
  1e3 + 5.0,
  1e4 + 5.0,
  1e5 + 5.0,
  1e6 + 5.0,
  1e7 + 5.0,
  1e8 + 5.0,
  1e9 + 5.0,
  1e10 + 5.0,
  1e11 + 5.0,
  1e12 + 5.0,
  1e15 + 5.0,
  1e18 + 5.0,
};

const std::array distanceLables = {
  "1e0 + 5.0",
  "1e2 + 5.0",
  "1e3 + 5.0",
  "1e4 + 5.0",
  "1e5 + 5.0",
  "1e6 + 5.0",
  "1e7 + 5.0",
  "1e8 + 5.0",
  "1e9 + 5.0",
  "1e10 + 5.0",
  "1e11 + 5.0",
  "1e12 + 5.0",
  "1e15 + 5.0",
  "1e18 + 5.0",
};

constexpr i32 TEST_FRAME_COUNT = 60;

template<typename T>
using TestArray = std::array<T, TEST_FRAME_COUNT>;
struct TestData
{
  TestArray<ComPtr<ID3D11Query>>            gpuDisjointQueries;
  TestArray<ComPtr<ID3D11Query>>            gpuStarts;
  TestArray<ComPtr<ID3D11Query>>            gpuEnds;
  TestArray<std::chrono::microseconds>      cpuTimes;
  TestArray<ComPtr<ID3D11Texture2D>>        testTargets;
  TestArray<ComPtr<ID3D11RenderTargetView>> testTargetViews;
};

struct TestRun
{
  TestData cpuDoubleData;
  TestData f32Data;
  TestData emulatedDoubleData;
  TestData parallaxData;
};

void RunTests(
  methods::F32Method               &f32Method,
  methods::CpuDoubleMethod         &cpuDoubleMethod,
  methods::GpuEmulatedDoubleMethod &gpuEmulatedDoubleMethod,
  methods::Parallax                &parallax,
  dx::RenderContext                &ctx,
  ShaderWatcher                    &shaderWatcher)
{
  TestRun testRun{};

  auto InitRenderTarget = [&ctx](TestData &data) {
    for (u32 i = 0; i < TEST_FRAME_COUNT; i++)
    {
      auto                &target     = data.testTargets[i];
      auto                &view       = data.testTargetViews[i];
      D3D11_TEXTURE2D_DESC targetDesc = {
        .Width     = 1920,
        .Height    = 1080,
        .MipLevels = 1,
        .ArraySize = 1,
        .Format    = DXGI_FORMAT_R8G8B8A8_UNORM,
        .SampleDesc =
          {
            .Count   = 1,
            .Quality = 0,
          },
        .Usage          = D3D11_USAGE_DEFAULT,
        .BindFlags      = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags = 0,
        .MiscFlags      = 0,
      };

      dx::ThrowIfFailed(ctx.device->CreateTexture2D(&targetDesc, nullptr, target.GetAddressOf()));
      dx::ThrowIfFailed(
        ctx.device->CreateRenderTargetView(target.Get(), nullptr, view.GetAddressOf()));

      auto &gpuDisjointQuery = data.gpuDisjointQueries[i];
      auto &gpuStart         = data.gpuStarts[i];
      // auto &gpuEnd;
    }
  };

  // for (i32 i = 0; i < TEST_FRAME_COUNT; i++)
  //{
  //   InitRenderTarget(
  //     testRun.cpuDoubleData.testTargets[i],
  //     testRun.cpuDoubleData.testTargetViews[i]);
  //   InitRenderTarget(testRun.f32Data.testTargets[i], testRun.f32Data.testTargetViews[i]);
  //   InitRenderTarget(
  //     testRun.emulatedDoubleData.testTargets[i],
  //     testRun.emulatedDoubleData.testTargetViews[i]);
  //   InitRenderTarget(testRun.parallaxData.testTargets[i],
  //   testRun.parallaxData.testTargetViews[i]);
  // }

  glm::dmat4 projection = glm::infinitePerspective(90.0, (double)WIDTH / (double)HEIGHT, 0.001);
  glm::dvec3 modelPos{0.0, 0.0, 0.0};
  glm::dvec3 sceneOrigin{0.0};

  const glm::dvec3 eyeStart    = {0.0, 0.0, 5.0};
  const glm::dvec3 targetStart = eyeStart + glm::dvec3{0.0, 0.0, -5.0};
  const glm::dvec3 up          = {0.0, 1.0, 0.0};
  ArcballCamera    arcballCamera{eyeStart, targetStart, up};
  modelPos.x    = distances[0];
  sceneOrigin.x = distances[0];
  arcballCamera = {sceneOrigin + eyeStart, sceneOrigin + targetStart, up};

  for (i32 i = 0; i < TEST_FRAME_COUNT; i++)
  {
    auto start = std::chrono::high_resolution_clock::now();
    cpuDoubleMethod.Update(ctx.context.Get(), projection * arcballCamera.transform(), modelPos);
    // cpuDoubleMethod.Draw(ctx, shaderWatcher);
    auto end = std::chrono::high_resolution_clock::now();
    testRun.cpuDoubleData.cpuTimes[i] =
      std::chrono::duration_cast<std::chrono::microseconds>(end - start);
  }
}

int main(int argc, char **argv)
{
  const dx::Window window = dx::CreateWin(WIDTH, HEIGHT, "win");

  dx::RenderContext ctx = dx::InitContext(window);

  auto c = glm::lookAt(glm::vec3{1e9, 0, 0}, {1e9, 0, -100}, {0, 1, 0});
  auto p = glm::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);

  auto vp = p * c;

  auto m   = glm::translate(glm::mat4(1.0), {1e9, 0, 0});
  auto mvp = vp * m;

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO &io = ImGui::GetIO();
  ImGui::StyleColorsClassic();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

  auto      t = glm::translate(glm::mat4(1.0), {42164000.0f, 0.0f, 0.0f});
  auto      s = glm::scale(glm::mat4(1.0), {1, 1, 1});
  glm::vec4 v{10.0f, 1.0f, 1.0f, 1.0f};

  auto r = t * s * v;
  printf("%f %f %f", r.x, r.y, r.z);

  ImGui_ImplSDL2_InitForD3D(window.win);
  ImGui_ImplDX11_Init(ctx.Device(), ctx.DeviceContext());

  f32 clearColor[] = {0.5, 0.5, 0.5, 1.0};

  ShaderWatcher shaderWatcher{ctx.Device()};

  glm::dmat4 projection = glm::infinitePerspective(90.0, (double)WIDTH / (double)HEIGHT, 0.001);

  const glm::dvec3 eyeStart    = {0.0, 0.0, 5.0};
  const glm::dvec3 targetStart = eyeStart + glm::dvec3{0.0, 0.0, -5.0};
  const glm::dvec3 up          = {0.0, 1.0, 0.0};
  ArcballCamera    arcballCamera{eyeStart, targetStart, up};

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
  Method      method{Method::Parallax};
  SceneModel  currentModel{};
  std::string currentModelPath;
  Model       model = LoadModel("models/suzanne.glb");
  Scene       scene{{model}};

  methods::F32Method               f32Method{ctx.Device(), shaderWatcher};
  methods::CpuDoubleMethod         cpuDoubleMethod{ctx.Device(), shaderWatcher};
  methods::GpuDoubleMethod         gpuDoubleMethod{ctx.Device(), shaderWatcher};
  methods::GpuEmulatedDoubleMethod emulatedDoubleMethod{ctx.Device(), shaderWatcher};
  methods::Parallax                parallaxMethod{ctx.Device(), shaderWatcher};
  f32Method.SetScene(scene);
  cpuDoubleMethod.SetScene(scene);
  gpuDoubleMethod.SetScene(scene);
  emulatedDoubleMethod.SetScene(scene);
  parallaxMethod.SetScene(scene);

  glm::dvec3 modelPos{0.0, 0.0, 0.0};
  glm::dmat4 modelTranslation = glm::identity<glm::dmat4>();
  glm::dvec3 sceneOrigin{0.0};

  i32  currentDistance = 0;
  bool runTests        = false;
  i32  testFrame       = 0;

  std::array<std::chrono::time_point<std::chrono::high_resolution_clock>, 60> edStarts;
  std::array<std::chrono::time_point<std::chrono::high_resolution_clock>, 60> edEnds;
  std::array<std::chrono::time_point<std::chrono::high_resolution_clock>, 60> imposterStarts;
  std::array<std::chrono::time_point<std::chrono::high_resolution_clock>, 60> imposterEnds;

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
#if 0
    else if (ImGui::RadioButton("GPU Double Method", method == Method::GpuDouble))
    {
      method = Method::GpuDouble;
    }
#endif
    else if (ImGui::RadioButton("Emulated Double Method", method == Method::GpuEmulatedDouble))
    {
      method = Method::GpuEmulatedDouble;
    }
    else if (ImGui::RadioButton("Parallax Method", method == Method::Parallax))
    {
      method = Method::Parallax;
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
        emulatedDoubleMethod.SetScene(scene);
        parallaxMethod.SetScene(scene);
      }
    }

    ImGui::Separator();

    // ImGui::InputDouble("Model Position X ", &modelPos.x);
    // ImGui::InputDouble("Model Position Y ", &modelPos.y);
    // ImGui::InputDouble("Model Position Z ", &modelPos.z);
    // ImGui::InputDouble("Scene Origin X ", &sceneOrigin.x);
    // ImGui::InputDouble("Scene Origin Y ", &sceneOrigin.y);
    // ImGui::InputDouble("Scene Origin Z ", &sceneOrigin.z);

    ImGui::ListBox("distances", &currentDistance, distanceLables.data(), distanceLables.size());
    if (ImGui::Button("Set Origin"))
    {
      modelPos.x    = distances[currentDistance];
      sceneOrigin.x = distances[currentDistance];
      arcballCamera = {sceneOrigin + eyeStart, sceneOrigin + targetStart, up};
    }

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
    if (ImGui::Button("run tests"))
    {
      // RunTests(f32Method, cpuDoubleMethod, emulatedDoubleMethod, parallaxMethod, ctx);
      runTests = true;
      method   = Method::CpuDouble;
    }

    if (runTests)
    {
      if (method == Method::GpuEmulatedDouble)
      {
        edStarts[testFrame] = std::chrono::high_resolution_clock::now();
      }
      else if (method == Method::Parallax)
      {
        imposterStarts[testFrame] = std::chrono::high_resolution_clock::now();
      }
    }
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
      cpuDoubleMethod.Draw(ctx, shaderWatcher, ctx.backbufferRTV.Get(), runTests, testFrame);
      break;
#if 0
    case Method::GpuDouble:
      gpuDoubleMethod.Update(
        ctx.DeviceContext(),
        projection * glm::dmat4{arcballCamera.transform()},
        glm::dvec3{modelPos});
      gpuDoubleMethod.Draw(ctx, shaderWatcher);
      break;
#endif
    case Method::GpuEmulatedDouble:
      emulatedDoubleMethod.Update(
        ctx.DeviceContext(),
        projection * glm::dmat4{arcballCamera.transform()},
        glm::dvec3{modelPos});
      emulatedDoubleMethod.Draw(ctx, shaderWatcher, ctx.backbufferRTV.Get(), runTests, testFrame);
      break;
    case Method::Parallax:
      parallaxMethod.Draw(
        WIDTH,
        HEIGHT,
        ctx.DeviceContext(),
        projection * glm::dmat4{arcballCamera.transform()},
        arcballCamera.transform(),
        projection,
        glm::dvec3{modelPos},
        glm::dvec3{arcballCamera.eye()},
        sceneOrigin,
        arcballCamera,
        ctx,
        shaderWatcher,
        ctx.backbufferRTV.Get(),
        runTests,
        testFrame);
      break;
    default:
      assert(0);
    }
    if (runTests)
    {
      if (method == Method::GpuEmulatedDouble)
      {
        edEnds[testFrame] = std::chrono::high_resolution_clock::now();
      }
      else if (method == Method::Parallax)
      {
        imposterEnds[testFrame] = std::chrono::high_resolution_clock::now();
      }
    }

    ImGui::End();

    ImGui::Render();
    // ctx.context->OMSetRenderTargets(1, &)
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

    ctx.swapchain->Present(1, 0);
    if (runTests)
    {
      testFrame++;
      if (testFrame >= 60)
      {
        if (method == Method::CpuDouble)
        {
          method = Method::GpuEmulatedDouble;
        }
        else if (method == Method::GpuEmulatedDouble)
        {
          method = Method::Parallax;
        }
        else if (method == Method::Parallax)
        {
          runTests                   = false;
          auto cpuDoubleMethodTiming = cpuDoubleMethod.GetTimingData(ctx.DeviceContext());
          auto emulatedDoubleTimings = emulatedDoubleMethod.GetTimingData(ctx.DeviceContext());
          auto imposterTimings       = parallaxMethod.GetTimingData(ctx.DeviceContext());

          std::vector<std::chrono::microseconds> edTimes;
          std::vector<std::chrono::microseconds> imposterTimes;

          for (size_t i = 0; i < 60; i++)
          {
            edTimes.push_back(
              std::chrono::duration_cast<std::chrono::microseconds>(edEnds[i] - edStarts[i]));
            imposterTimes.push_back(std::chrono::duration_cast<std::chrono::microseconds>(
              imposterEnds[i] - imposterStarts[i]));
          }
          testFrame = 0;
        }
        testFrame = 0;
      }
    }
  }

  ImGui_ImplDX11_Shutdown();
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

  SDL_DestroyWindow(window.win);
  SDL_Quit();

  return 0;
}

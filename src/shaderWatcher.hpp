#pragma once

#include "dx.hpp"
#include "utils.hpp"

#include <d3d11_3.h>
#include <filesystem>
#include <vector>

using RenderProgramHandle  = u32;
using ComputeProgramHandle = u32;

struct RenderProgram
{
  ID3D11VertexShader *vertexShader;
  ID3D11PixelShader  *pixelShader;
};

template<typename T>
struct WatchPair
{
  std::vector<std::string>                     sourcePaths;
  std::vector<ComPtr<T>>                       shaders;
  std::vector<std::filesystem::file_time_type> times;

  void Add(std::string sourcePath, ComPtr<T> shader, std::filesystem::file_time_type time)
  {
    sourcePaths.emplace_back(std::move(sourcePath));
    shaders.emplace_back(shader);
    times.emplace_back(time);
  }

  void Set(u32 i, ComPtr<T> shader, std::filesystem::file_time_type time)
  {
    assert(i < shaders.size());
    shaders[i] = shader;
    times[i]   = time;
  }
};

class ShaderWatcher final
{
  RenderProgramHandle  mNextRenderProgram{};
  ComputeProgramHandle mNextComputeProgram{};

  WatchPair<ID3D11VertexShader>  mVertexShaders;
  WatchPair<ID3D11PixelShader>   mPixelShaders;
  WatchPair<ID3D11ComputeShader> mComputeShaders;
  ID3D11Device3                 *mDevice;

public:
  explicit ShaderWatcher(ID3D11Device3 *device) : mDevice{device}
  {
  }

  RenderProgramHandle RegisterShader(const std::string &vertexPath, const std::string &pixelPath);

  ComputeProgramHandle RegisterShader(const std::string &computePath);

  RenderProgram        GetRenderProgram(RenderProgramHandle handle);
  ID3D11ComputeShader *GetComputeProgram(ComputeProgramHandle handle);
};
#include "shaderWatcher.hpp"

#include <functional>
#include <print>

#ifndef NDEBUG
#define DEBUG
#endif

namespace
{

ComPtr<ID3DBlob>
CompileShader(const std::string &path, const char *entry_point, const char *shader_model)
{
  const std::string source = io::ReadFile(path);

  ID3DBlob        *binary = nullptr;
  ComPtr<ID3DBlob> errors;

#ifdef DEBUG
  u32 flags = D3DCOMPILE_DEBUG;
#else
  u32 flags = 0;
#endif
  flags |= D3DCOMPILE_SKIP_OPTIMIZATION | D3DCOMPILE_IEEE_STRICTNESS | D3DCOMPILE_ENABLE_STRICTNESS;

  HRESULT res = D3DCompile(
    source.data(),
    source.size(),
    nullptr,
    nullptr,
    nullptr,
    entry_point,
    shader_model,
    flags,
    0,
    &binary,
    &errors);

  if (FAILED(res))
  {
    std::println("Shader Error: {}", (const char *)errors->GetBufferPointer());
    return nullptr;
  }
  else
  {
    return binary;
  }
}

struct CompiledVert
{
  ComPtr<ID3D11VertexShader> shader;
  ComPtr<ID3D11InputLayout>  layout;
};

CompiledVert CompileVert(
  const std::string                           &path,
  ID3D11Device3                               *device,
  const std::vector<D3D11_INPUT_ELEMENT_DESC> &inputDesc)
{
  ComPtr<ID3DBlob> binary = CompileShader(path, "VSMain", "vs_5_0");
  if (binary == nullptr)
  {
    return {};
  }
  ComPtr<ID3D11VertexShader> shader;
  ComPtr<ID3D11InputLayout>  layout;
  dx::ThrowIfFailed(device->CreateInputLayout(
    inputDesc.data(),
    inputDesc.size(),
    binary->GetBufferPointer(),
    binary->GetBufferSize(),
    layout.GetAddressOf()));

  device->CreateVertexShader(
    binary->GetBufferPointer(),
    binary->GetBufferSize(),
    nullptr,
    shader.GetAddressOf());
  return {shader, layout};
}

ComPtr<ID3D11PixelShader> CompilePixel(const std::string &path, ID3D11Device3 *device)
{
  ComPtr<ID3DBlob> binary = CompileShader(path, "PSMain", "ps_5_0");
  if (binary == nullptr)
  {
    return nullptr;
  }
  ComPtr<ID3D11PixelShader> shader;
  device->CreatePixelShader(
    binary->GetBufferPointer(),
    binary->GetBufferSize(),
    nullptr,
    shader.GetAddressOf());
  return shader;
}

ComPtr<ID3D11ComputeShader> CompileCompute(const std::string &path, ID3D11Device3 *device)
{
  ComPtr<ID3DBlob>            binary = CompileShader(path, "CSMain", "cs_5_0");
  ComPtr<ID3D11ComputeShader> shader;
  device->CreateComputeShader(
    binary->GetBufferPointer(),
    binary->GetBufferSize(),
    nullptr,
    shader.GetAddressOf());
  return shader;
}

template<typename T, typename CompFunc>
void RecompileIfChanged(WatchPair<T> &shaders, CompFunc compFunc, ID3D11Device3 *device, u32 handle)
{
  const std::string &path      = shaders.sourcePaths[handle];
  auto               writeTime = std::filesystem::last_write_time(std::filesystem::path{path});
  if (writeTime > shaders.times[handle])
  {
    shaders.Set(handle, compFunc(shaders.sourcePaths[handle], device), writeTime);
  }
}

} // namespace

RenderProgramHandle ShaderWatcher::RegisterShader(
  const std::string                           &vertexPath,
  const std::string                           &pixelPath,
  const std::vector<D3D11_INPUT_ELEMENT_DESC> &inputDesc)
{
  RenderProgramHandle handle = mNextRenderProgram;
  mNextRenderProgram++;
  auto           p    = std::filesystem::current_path();
  ::CompiledVert vert = ::CompileVert(vertexPath, mDevice, inputDesc);
  mInputDescs.emplace_back(inputDesc);
  mVertexInputLayouts.emplace_back(vert.layout);
  mVertexShaders.Add(
    vertexPath,
    vert.shader,
    std::filesystem::last_write_time(std::filesystem::path{vertexPath}));
  mPixelShaders.Add(
    pixelPath,
    ::CompilePixel(pixelPath, mDevice),
    std::filesystem::last_write_time(std::filesystem::path{pixelPath}));

  return handle;
}

ComputeProgramHandle ShaderWatcher::RegisterShader(const std::string &computePath)
{
  ComputeProgramHandle handle = mNextComputeProgram;
  mNextComputeProgram++;
  mComputeShaders.Add(
    computePath,
    ::CompileCompute(computePath, mDevice),
    std::filesystem::last_write_time(std::filesystem::path{computePath}));
  return handle;
}

RenderProgram ShaderWatcher::GetRenderProgram(RenderProgramHandle handle)
{
  assert(handle < mVertexShaders.shaders.size());
  assert(handle < mPixelShaders.shaders.size());
#ifdef DEBUG
  ::RecompileIfChanged(
    mVertexShaders,
    [this, handle](const std::string &path, ID3D11Device3 *device) {
      ::CompiledVert shader       = CompileVert(path, device, mInputDescs[handle]);
      mVertexInputLayouts[handle] = shader.layout;
      return shader.shader;
    },
    mDevice,
    handle);
  ::RecompileIfChanged(mPixelShaders, CompilePixel, mDevice, handle);
#endif
  return {
    .vertexShader = mVertexShaders.shaders[handle].Get(),
    .pixelShader  = mPixelShaders.shaders[handle].Get(),
    .inputLayout  = mVertexInputLayouts[handle].Get(),
  };
}

ID3D11ComputeShader *ShaderWatcher::GetComputeProgram(ComputeProgramHandle handle)
{
  assert(handle < mComputeShaders.shaders.size());
#ifdef DEBUG
  ::RecompileIfChanged(mComputeShaders, CompileCompute, mDevice, handle);
#endif
  return mComputeShaders.shaders[handle].Get();
}

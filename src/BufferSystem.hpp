#pragma once

#include "Handle.hpp"
#include "dx.hpp"

#include <d3d11_3.h>
#include <span>
#include <vector>

namespace kronk
{

class BufferSystem;

MAKE_HANDLE(Buffer);

class Buffer
{
  static inline BufferSystem *sBufferSystem;
  friend class BufferSystem;

  BufferHandle mHandle{};

  static void InitStatics(BufferSystem *bufferSystem)
  {
    sBufferSystem = bufferSystem;
  }

  explicit Buffer(BufferHandle handle) : mHandle{handle}
  {
  }

public:
  Buffer() = default;
  u32 GetSize() const;

  void Update();
};

template<typename T>
concept IsIndex = std::same_as<u16, T> || std::same_as<u32, T>;

class BufferSystem
{
  ID3D11Device3 *mDevice;

  std::vector<ComPtr<ID3D11Buffer>> mBuffers;

  std::vector<u32> mSizes;
  std::vector<u32> mStrides;
  std::vector<u32> mGen;
  std::vector<u32> mFreeList;
  BufferHandle     mNext;

public:
  explicit BufferSystem(ID3D11Device3 *device) : mDevice{device}
  {
  }

  template<typename T>
  Buffer CreateConstantBuffer(u32 size, const T &data)
  {
    return CreateBuffer(
      {
        .ByteWidth           = dx::GpuSizeof<T>(),
        .Usage               = D3D11_USAGE_DYNAMIC,
        .BindFlags           = D3D11_BIND_CONSTANT_BUFFER,
        .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags           = {},
        .StructureByteStride = {},
      },
      reinterpret_cast<void *>(&data));
  }

  template<typename T>
  Buffer CreateStructuredBuffer(u32 size, u32 stride, std::span<const T> data)
  {
    assert(size == data.size_bytes());
    return CreateBuffer(
      {
        .ByteWidth           = size,
        .Usage               = D3D11_USAGE_DYNAMIC,
        .BindFlags           = D3D11_BIND_SHADER_RESOURCE,
        .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags           = D3D11_RESOURCE_MISC_BUFFER_STRUCTURED,
        .StructureByteStride = stride,
      },
      reinterpret_cast<void *>(data.data()));
  }

  template<IsIndex T>
  Buffer CreateIndexBuffer(u32 size, std::span<const T> data)
  {
    assert(size == data.size_bytes());
    return CreateBuffer(
      {
        .ByteWidth           = size,
        .Usage               = D3D11_USAGE_DYNAMIC,
        .BindFlags           = D3D11_BIND_INDEX_BUFFER,
        .CPUAccessFlags      = D3D11_CPU_ACCESS_WRITE,
        .MiscFlags           = {},
        .StructureByteStride = {},
      },
      reinterpret_cast<void *>(data.data()))
  }

  u32  GetBufferSize(BufferHandle handle) const;
  void Update(BufferHandle handle);

private:
  Buffer CreateBuffer(const D3D11_BUFFER_DESC &desc, void *data);
};

} // namespace kronk
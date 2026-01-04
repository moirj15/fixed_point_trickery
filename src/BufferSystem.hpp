#pragma once

#include "Handle.hpp"
#include "dx.hpp"

#include <d3d11_3.h>
#include <functional>
#include <span>
#include <vector>
#if defined(max)
#undef max
#endif

namespace kronk
{

class BufferSystem;

MAKE_HANDLE(Buffer);

using BufferUpdateCallback = std::function<void(std::span<const std::byte>)>;

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

  void Update(BufferUpdateCallback callback);

  void Destroy();
};

template<typename T>
concept IsIndex = std::same_as<u16, T> || std::same_as<u32, T>;

class BufferSystem
{
  static constexpr size_t MAX_SLOTS = std::numeric_limits<u16>::max();

  ID3D11Device3        *mDevice;
  ID3D11DeviceContext3 *mContext;

  std::vector<ComPtr<ID3D11Buffer>> mBuffers;

  std::vector<u32> mSizes;
  std::vector<u32> mStrides;
  std::vector<u32> mGen;
  std::vector<u32> mFreeList{};
  BufferHandle     mNext{0, 0};

public:
  explicit BufferSystem(ID3D11Device3 *device, ID3D11DeviceContext3 *context) :
      mDevice{device},
      mContext{context},
      mBuffers(MAX_SLOTS),
      mSizes(MAX_SLOTS),
      mStrides(MAX_SLOTS),
      mGen(MAX_SLOTS)
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
      reinterpret_cast<void *>(data.data()));
  }

  u32  GetBufferSize(BufferHandle handle) const;
  void UpdateBuffer(BufferHandle handle, BufferUpdateCallback callback);
  void DestroyBuffer(BufferHandle handle);

private:
  Buffer CreateBuffer(const D3D11_BUFFER_DESC &desc, void *data);
};

} // namespace kronk
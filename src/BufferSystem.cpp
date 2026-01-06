
#include "BufferSystem.hpp"
namespace kronk
{

u32 BufferSystem::GetBufferSize(BufferHandle handle) const
{
  assert(mGen[handle.GetValue()] == handle.GetGen());
  return mSizes[handle.GetValue()];
}

ID3D11ShaderResourceView *BufferSystem::GetView(BufferHandle handle) const
{
  assert(mGen[handle.GetValue()] == handle.GetGen());
  return mViews[handle.GetValue()].Get();
}

void BufferSystem::UpdateBuffer(BufferHandle handle, BufferUpdateCallback callback)
{
  assert(mGen[handle.GetValue()] == handle.GetGen());
  const u32                index  = handle.GetValue();
  ID3D11Buffer            *buffer = mBuffers[index].Get();
  D3D11_MAPPED_SUBRESOURCE mappedResource{};

  dx::ThrowIfFailed(mContext->Map(buffer, 0, D3D11_MAP_WRITE, 0, &mappedResource));
  {
    std::span mappedData{reinterpret_cast<std::byte *>(mappedResource.pData), mSizes[index]};
    callback(mappedData);
  }
  mContext->Unmap(buffer, 0);
}

void BufferSystem::DestroyBuffer(BufferHandle handle)
{
  assert(mGen[handle.GetValue()] == handle.GetGen());
  mBuffers[handle.GetValue()].Reset();
  mGen[handle.GetValue()]++;
  mFreeList.push_back(handle.GetValue());
}

Buffer BufferSystem::CreateBuffer(const D3D11_BUFFER_DESC &desc, const void *data)
{
  ID3D11Buffer *buffer{};
  if (data == nullptr)
  {
    mDevice->CreateBuffer(&desc, nullptr, &buffer);
  }
  else
  {
    D3D11_SUBRESOURCE_DATA d{};
    d.pSysMem = data;
    mDevice->CreateBuffer(&desc, &d, &buffer);
  }
  const u32    index = mNext.GetValue();
  BufferHandle handle{index, mGen[index]};

  mSizes[index]   = desc.ByteWidth;
  mStrides[index] = desc.StructureByteStride;
  mGen[index]     = 0;

  mNext.IncHandle();
  return Buffer{handle};
}

u32 Buffer::GetSize() const
{
  return sBufferSystem->GetBufferSize(mHandle);
}

ID3D11ShaderResourceView *Buffer::GetView() const
{
  return sBufferSystem->GetView(mHandle);
}

void Buffer::Update(BufferUpdateCallback callback)
{
  sBufferSystem->UpdateBuffer(mHandle, callback);
}

void Buffer::Destroy()
{
  sBufferSystem->DestroyBuffer(mHandle);
}

} // namespace kronk

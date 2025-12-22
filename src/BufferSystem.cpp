
#include "BufferSystem.hpp"
namespace kronk
{


u32 BufferSystem::GetBufferSize(BufferHandle handle) const
{
  return u32();
}

void BufferSystem::Update(BufferHandle handle)
{
}

Buffer BufferSystem::CreateBuffer(const D3D11_BUFFER_DESC &desc, void *data)
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
}

} // namespace kronk

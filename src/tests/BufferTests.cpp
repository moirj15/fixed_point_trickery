#include "../BufferSystem.hpp"
#include "D3DFixture.hpp"

#include <catch2/catch_test_macros.hpp>

namespace kronk::tests
{

#if 0
TEST_CASE_METHOD(D3D11Fixture, "")
{
}
#endif

TEST_CASE_METHOD(D3D11Fixture, "BufferSystem init smoke test")
{
  BufferSystem{mContext.m_device.Get(), mContext.m_context.Get()};
}

TEST_CASE_METHOD(D3D11Fixture, "BufferSystem basic CreateConstantBuffer call")
{
  BufferSystem bs{mContext.m_device.Get(), mContext.m_context.Get()};

  Buffer buffer = bs.CreateConstantBuffer(sizeof(u32), 42);
}

} // namespace kronk::tests

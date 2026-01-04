
#pragma once

#include "utils.hpp"

#include <cassert>
#include <numeric>

namespace kronk
{

template<typename Tag>
class Handle final
{
  static constexpr u32 INVALID = std::numeric_limits<u32>::max();

  u32 mHandle{std::numeric_limits<u32>::max()};
  u32 mGen{std::numeric_limits<u32>::max()};

public:
  Handle() = default;
  explicit Handle(u32 handle, u32 gen) : mHandle{handle}, mGen{gen}
  {
  }

  u32 GetValue() const
  {
    return mHandle;
  }

  u32 GetGen() const
  {
    return mGen;
  }

  void IncHandle()
  {
    assert(mHandle != INVALID);
    mHandle++;
  }

  void IncGen()
  {
    assert(mGen != INVALID);
    mGen++;
  }
};

#define MAKE_HANDLE(name)                                                                          \
  struct __##name##TAG                                                                             \
  {                                                                                                \
  };                                                                                               \
  using name##Handle = Handle<__##name##TAG>;

MAKE_HANDLE(foo);

} // namespace kronk

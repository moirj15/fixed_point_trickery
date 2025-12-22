
#pragma once

#include "utils.hpp"

#include <numeric>

namespace kronk
{

template<typename Tag>
class Handle final
{
  u32 mHandle{std::numeric_limits<u32>::max()};
  u32 mGen{std::numeric_limits<u32>::max()};

public:
  Handle() = default;
  explicit Handle(u32 handle) : mHandle{handle}, mGen{}
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
};

#define MAKE_HANDLE(name)                                                                           \
  struct __##name##TAG                                                                             \
  {                                                                                                \
  };                                                                                               \
  using name##Handle = Handle<__##name##TAG>;

MAKE_HANDLE(foo);

} // namespace kronk

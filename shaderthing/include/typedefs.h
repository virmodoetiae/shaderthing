#pragma once

#include <functional>
#include <vector>
#include "vir/include/vpointers.h"

namespace ShaderThing
{

template<typename T>
using UPtr = vir::UniquePtr<T>;

template<typename T>
using UPtrVector = std::vector<vir::UniquePtr<T>>;

}
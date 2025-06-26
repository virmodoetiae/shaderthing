#pragma once

#include <functional>
#include <vector>
#include "vir/include/vpointers.h"
#include "vir/include/vgraphics/vcore/vbuffers.h"

namespace ShaderThing
{

template<typename T>
using UPtr = vir::UniquePtr<T>;

template<typename T>
using UPtrVector = std::vector<vir::UniquePtr<T>>;

typedef vir::TextureBuffer::WrapMode       WrapMode;
typedef vir::TextureBuffer::FilterMode     FilterMode;
typedef vir::TextureBuffer::InternalFormat InternalFormat;

}
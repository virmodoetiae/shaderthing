#pragma once

#include <functional>
#include <vector>
#include "vir/include/vpointers.h"
#include "vir/include/vgraphics/vcore/vbuffers.h"
#include "vir/include/vgraphics/vpostprocess/vquantizer.h"
#include "vir/include/vgraphics/vmisc/vgifencoder.h"

namespace ShaderThing
{

template<typename T>
using GPtr = vir::GlobalPtr<T>;

template<typename T>
using UPtr = vir::UniquePtr<T>;

template<typename T>
using WPtr = vir::WeakPtr<T>;

template<typename T>
using UPtrVector = std::vector<vir::UniquePtr<T>>;

template<typename T>
using WPtrVector = std::vector<vir::WeakPtr<T>>;

typedef vir::TextureBuffer::WrapMode         WrapMode;
typedef vir::TextureBuffer::FilterMode       FilterMode;
typedef vir::TextureBuffer::InternalFormat   InternalFormat;
typedef vir::Quantizer::Settings::DitherMode DitherMode;
typedef vir::GifEncoder::PaletteMode         PaletteMode;
typedef vir::TextureBuffer::ImageBindMode    ImageBindMode;
typedef vir::TextureBuffer::DataType         DataType;

}
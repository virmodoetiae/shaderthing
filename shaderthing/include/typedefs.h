/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2026 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

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
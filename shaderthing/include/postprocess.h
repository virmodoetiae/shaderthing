/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <string>
#include <unordered_map>

#include "shaderthing/include/macros.h"

#include "vir/include/vir.h"

typedef vir::PostProcess::Type Type;

namespace ShaderThing
{

class Layer;
class ObjectIO;

class PostProcess
{
protected:

    // Source layer
    Layer*             inputLayer_        = nullptr;
    vir::Framebuffer** inputFramebuffer_  = nullptr;
    // Native post-process which also holds the target output framebuffer
    vir::PostProcess*  native_            = nullptr;
    bool               isActive_          = false;

    DELETE_COPY_MOVE(PostProcess)

    PostProcess
    (
        Layer* inputLayer, 
        vir::PostProcess* nativePostProcess
    );

public:
    
    virtual ~PostProcess();

    static PostProcess* create
    (
        Layer* inputLayer, 
        Type type
    );
    
    static void loadStaticData(const ObjectIO& io);
    static void saveStaticData(ObjectIO& io);

    static PostProcess* load
    ( 
        const ObjectIO& io,
        Layer* inputLayer
    );
    virtual void save(ObjectIO& writer) const = 0;
    virtual void run() = 0;
    virtual void renderGui() = 0;

    // Return access to the output framebuffer with the applied post-processing
    // effect
    vir::Framebuffer* outputFramebuffer(){return native_->output();}

    // Assign the address of the post-processing output framebuffer to the 
    // input layer writeonly framebuffer. In this way, the post-processed
    // changes are instantly visible to all resources using the input layer
    // as an input of their own. This modification is overwritten anyways
    // at the start of the input layer's rendering step, during front and
    // back-buffer swapping
    void overwriteInputLayerFramebuffer();

    Type type() const {return native_->type();}
    bool canRunOnDeviceInUse() const {return native_->canRunOnDeviceInUse();}
    const std::string& errorMessage() const {return native_->errorMessage();}
    const std::string& name() const 
    {
        return vir::PostProcess::typeToName.at(type());
    }
    bool isActive() const {return isActive_;}
    Layer* inputLayer() const {return inputLayer_;}
    void setActive(bool flag){isActive_ = flag;}
};

//----------------------------------------------------------------------------//

class QuantizationPostProcess : public PostProcess
{
protected:

    typedef vir::Quantizer   nativeType;
    typedef nativeType::Settings Settings;

    vir::Quantizer::Settings settings_ =
    {
        nullptr,                      // paletteData
        true,                         // reseedPalette
        true,                         // recalculatePalette
        .707f,                        // relTol
        Settings::DitherMode::None,
        .25f,                         // ditherThreshold
        Settings::IndexMode::Default,
        -1,                           // alphaCutoff
        true,                         // fastKMeans
        true,                         // regenerateMipMap
        false,                        // overwriteInput
        0                             // inputUnit
    };
    
    struct Palette
    {
        unsigned int     nColors = 0;
        // Palette data of size 3*size, with each value in the [0,255] range
        unsigned char*   data = nullptr;
        std::string      name = "New palette";
        Palette(){}
        Palette(const Palette& palette);
        Palette
        (
            std::initializer_list<unsigned int> colors, 
            const char* name = "New palette"
        );
        Palette& operator=(const Palette& rhs);
        ~Palette();
        void clear();
        bool operator==(const Palette& rhs);
        bool operator!=(const Palette& rhs){return !this->operator==(rhs);}
    };
    
    Palette                     currentPalette_ = {};
    static std::vector<Palette> userPalettes_;
    static std::vector<Palette> defaultPalettes_;

    // True if the palette has been manually modified via the embedded color
    // picker in the quantization tool (can be accessed by clicking any color
    // in the displayed palette), regardless of palette size changes
    bool                 paletteModified_     = false;
    // True if the palette size has been modified
    bool                 paletteSizeModified_ = true;
    bool                 refreshPalette_      = false;

public:
    
    QuantizationPostProcess(Layer* inputLayer);

    static QuantizationPostProcess* load
    (
        const ObjectIO& io,
        Layer* inputLayer
    );
    void save(ObjectIO& writer) const override;
    void run() override;
    void renderGui() override;

    static void saveUserPalettes(ObjectIO& io);
    static void loadUserPalettes(const ObjectIO& io);
};

//----------------------------------------------------------------------------//

class BloomPostProcess : public PostProcess
{
protected:

    typedef vir::Bloomer   nativeType;
    typedef nativeType::Settings Settings;

    vir::Bloomer::Settings settings_ = {};

public:
    
    BloomPostProcess(Layer* inputLayer);

    static BloomPostProcess* load
    (
        const ObjectIO& io,
        Layer* inputLayer
    );
    void save(ObjectIO& writer) const override;
    void run() override;
    void renderGui() override;
};

//----------------------------------------------------------------------------//

class BlurPostProcess : public PostProcess
{
protected:

    typedef vir::Blurrer   nativeType;
    typedef nativeType::Settings Settings;

    vir::Blurrer::Settings settings_         = {};
    bool                   isKernelCircular_ = true;

public:
    
    BlurPostProcess(Layer* inputLayer);

    static BlurPostProcess* load
    (
        const ObjectIO& io,
        Layer* inputLayer
    );
    void save(ObjectIO& writer) const override;
    void run() override;
    void renderGui() override;
};

}
/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#ifndef ST_QUANTIZER_TOOL_H
#define ST_QUANTIZER_TOOL_H

#include <unordered_map>

#include "vir/include/vir.h"

namespace ShaderThing
{

// Forward declarations
class ShaderThingApp;
class Layer;
class ObjectIO;

class QuantizationTool
{
public :

    static std::unordered_map<int, std::string> ditheringLevelToName;

protected:

    // Ref to top-level app
    ShaderThingApp& app_;

    // True if this tool's GUI is currently open
    bool isGuiOpen_;

    // True if this tool's GUI automatically opens when hovering over its entry
    // in the main application menu bar
    bool isGuiInMenu_;

    // True if quantization is active for the target layer
    bool isActive_;

    // True when the target layer is being quantizer for the first time. This
    // is used for optimization, as a (somewhat expensive) palettes
    // initialization in the style of the KMeans++ initialization algorithm is
    // performed on  first quantization. Otherwise, the latest available palette
    // is used as a guess palette for the KMeans clustering algorithm. More
    // details in vir/include/vgraphics/vmisc/vopengl/vopenglkmeansquantizer.h
    bool firstQuantization_;

    // True if the palette for the quantization of the target layer is
    // recomputed on every frame
    bool autoUpdatePalette_;

    // True if the alpha channel of the target layer is to be binarily set to
    // either 0 or 1 depending on the value of the original alpha channel
    bool isAlphaCutoff_;

    // Size of the palette (number of RGB colors) used for quantization
    int paletteSize_;

    // Level of dithering. 0 is no dithering, 1 is ordered dithering with a
    // 2x2 kernel, 2 is dithering with a 4x4 kernel.
    // This currently uses an int instead of a
    // vir::Quantizer::Settings::DitherMode enum because of legacy
    // implementation reasons, should be updated at some point
    int ditheringLevel_;

    // Fidelity of the clustering (KMeans) algorithm in a [0.,1.] range. The
    // higher the value, the lower the quantization error arising from the
    // computed quantization palette used for clustering, for a given palette
    // size
    float clusteringFidelity_;

    // Dithering threshold in a [0.,1.] range. The higher the value, the more
    // dithered the quantizer image will be
    float ditheringThreshold_;

    // Transparency cutoff threshold in a [0,255] range. If isAlphaCutoff_ is
    // true, pixels with alpha channel below this value will be rendered as fully
    // transparent, fully opaque otherwise, during the quantization step
    int alphaCutoffThreshold_;

    // Reference to native vir KMean quantizer
    vir::Quantizer* quantizer_;

    // Palette data of size 3*paletteSize_, with each value in the [0,255] range
    unsigned char* uIntPalette_;

    // Same as uIntPalette_, but with each value in the [0.,1.] range
    float* floatPalette_;

    // True if the palette has been manually modified via the embedded color
    // picker in the quantization tool (can be accessed by clicking any color
    // in the displayed palette)
    bool paletteModified_;

    // Ptr to target layer which is to be quantizer
    Layer* targetLayer_;

public:

    QuantizationTool(ShaderThingApp& app);
    ~QuantizationTool();

    // If the provided layer is the current targetLayer_, targetLayer_ is
    // reset to nullptr (no Layer deletion actually happens). Otherwise, nothing
    // happens
    void removeLayerAsTarget(Layer* layer);
    
    // If the provided layer is the current targetLayer_, said layer is
    // quantized with the current tool settings
    void quantize(Layer* layer);

    // Reset and/or de-allocate all object members to default values
    void reset();

    //
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}

    // Render the QuantizationTool GUI
    void renderGui();

    // Re-initialize all object members from the data stored in the provided
    // reader object. An ObjectIO object is fundamentally a JSON file in a C++
    // context
    void loadState(const ObjectIO& reader);

    // Serialize all object members to the provided writer object, which is
    // to be written to disk. An ObjectIO object is fundamentally a JSON file
    // in a C++ context
    void saveState(ObjectIO& writer);

    // Returns false if the QuantizationTool cannot run on the current hardware.
    // Related to the fact that the underlying vir::OpenGLQuantizer
    // requires OpenGL version 4.3 or greater to run (while ShaderThing can run
    // on OpenGL version 3.3 or higher)
    bool canRunOnDeviceInUse();

    // Returns the error message related to the inability of the
    // QuantizationTool to run if canRunOnDeviceInUse() returns false
    const std::string& errorMessage();

    // Accessors
    bool* isGuiOpenPtr() {return &isGuiOpen_;}
    bool isGuiInMenu() {return isGuiInMenu_;}
    Layer*& targetLayer() {return targetLayer_;}
};

}

#endif
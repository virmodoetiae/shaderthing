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

class ShaderThingApp;
class Layer;
class ObjectIO;

class QuantizationTool
{
public :

    static std::unordered_map<int, std::string> ditheringLevelToName;

protected:

    ShaderThingApp& app_;
    bool isGuiOpen_;
    bool isGuiInMenu_;
    bool isActive_;
    bool firstQuantization_;
    bool autoUpdatePalette_;
    bool isAlphaCutoff_;
    int paletteSize_;
    int ditheringLevel_;
    float clusteringFidelity_;
    float ditheringThreshold_;
    int alphaCutoffThreshold_;

    vir::KMeansQuantizer* quantizer_;
    unsigned char* uIntPalette_;
    float* floatPalette_;
    bool paletteModified_;

    Layer* targetLayer_;
    std::vector<Layer*>& layers_;

public:

    QuantizationTool(ShaderThingApp& app);
    ~QuantizationTool();

    void removeLayerAsTarget(Layer* layer);
    
    void quantize(Layer* layer);
    void update();
    void reset();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    void loadState(const ObjectIO& reader);
    void saveState(ObjectIO& writer);

    bool canRunOnDeviceInUse();
    const std::string& errorMessage();
    bool* isGuiOpenPtr() {return &isGuiOpen_;}
    bool isGuiInMenu() {return isGuiInMenu_;}
    Layer*& targetLayer() {return targetLayer_;}
};

}

#endif
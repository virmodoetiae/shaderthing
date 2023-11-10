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

    void loadState(std::string& source, uint32_t& index);
    void loadState(const ObjectIO& reader);
    void saveState(std::ofstream&);
    void saveState(ObjectIO& writer);
    
    void quantize(Layer* layer);
    void update();
    void reset();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    bool canRunOnDeviceInUse();
    const std::string& errorMessage();
    bool* isGuiOpenPtr() {return &isGuiOpen_;}
    bool isGuiInMenu() {return isGuiInMenu_;}
    Layer*& targetLayer() {return targetLayer_;}

    void getPalette(unsigned char*& data, unsigned int& dataSize)
    {
        quantizer_->getPalette(data, true);
        dataSize = 3*quantizer_->paletteSize();
    }

    // Serialization
    /*
    template<typename RapidJSONWriterType>
    void saveState(RapidJSONWriterType& writer)
    {
        if (targetLayer_ == nullptr)
            return;

        writer.String("quantizer");
        writer.StartObject();

        writer.String("active");
        writer.Bool(isActive_);

        writer.String("targetLayer");
        writer.String(targetLayer_->name().c_str());

        writer.String("ditheringLevel");
        writer.Int((int)ditheringLevel_);

        writer.String("ditheringThreshold");
        writer.Double(ditheringThreshold_);

        writer.String("clusteringFidelity");
        writer.Double(clusteringFidelity_);

        writer.String("transparencyCutoff");
        writer.Bool(isAlphaCutoff_);

        writer.String("transparencyCutoffThreshold");
        writer.Int(alphaCutoffThreshold_);

        writer.String("dynamicPalette");
        writer.Bool(autoUpdatePalette_);

        if (uIntPalette_ != nullptr)
        {
            writer.String("palette");
            writer.StartObject();
            writer.String("dataSize");
            writer.Int(3*paletteSize_);
            writer.String("data");
            writer.String((const char*)uIntPalette_, 3*paletteSize_, false);
            writer.EndObject(); // End of palette
        }

        writer.EndObject(); // End of quantizer
    }
    */
};

}

#endif
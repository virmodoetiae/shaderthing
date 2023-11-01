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

#ifndef ST_LAYER_MANAGER_H
#define ST_LAYER_MANAGER_H

#include <string>
#include <vector>

#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

class ShaderThingApp;
class Layer;
class Resource;

class LayerManager
{
protected:

    friend class Layer;
    static unsigned int nLayersSinceNewProject_;
    ShaderThingApp& app_;
    std::vector<Layer*> layers_;
    Layer* activeGuiLayer_;

    Layer* addLayer();

public:

    LayerManager(ShaderThingApp& app);
    ~LayerManager();
    
    void removeResourceFromUniforms(Resource* resource);

    void markAllShadersForCompilation();
    void clearFramebuffers();
    
    void reset();

    bool update();

    void renderGui();

    void renderLayers
    (
        vir::Framebuffer* target, 
        unsigned int nRenderPasses
    );

    void saveState(std::ofstream& file);
    void loadState(std::string& source, uint32_t& index);
    void preLoadAdjustment();

    std::vector<Layer*>& layersRef(){return layers_;}

    Layer*& activeGuiLayer(){return activeGuiLayer_;}
    void setActiveGuiLayer(Layer* layer){activeGuiLayer_ = layer;}
    void setTargetResolution(const glm::ivec2& resolution);

    // Serialization
    template<typename RapidJSONWriterType>
    void saveState(RapidJSONWriterType& writer)
    {
        writer.String("layers");
        writer.StartObject();
        for (auto layer : layers_)
            layer->saveState(writer);
        writer.EndObject();
    }

};

}

#endif
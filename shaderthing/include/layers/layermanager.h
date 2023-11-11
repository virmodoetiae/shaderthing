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
class ObjectIO;

class LayerManager
{
protected:

    friend class Layer;
    static unsigned int nLayersSinceNewProject_;
    ShaderThingApp& app_;
    std::vector<Layer*> layers_;
    Layer* activeGuiLayer_;

    Layer* addLayer();

    void markAllShadersForCompilation();

public:

    LayerManager(ShaderThingApp& app);
    ~LayerManager();
    
    void removeResourceFromUniforms(Resource* resource);
    
    void clearFramebuffers();
    
    void reset();

    bool update();

    void renderGui();

    void renderLayers
    (
        vir::Framebuffer* target, 
        unsigned int nRenderPasses
    );

    // Because of ImGui stuff I do not grasp, if the project one is loading
    // has a certain number of layers with a certain set of names that
    // are equal to the currently opened project layer names, the order of the 
    // newly loaded project's layer tabs (and thus the order of their rendering)
    // will be randomly reshuffled.
    // To avoid such a situation, the existing layers of the to-be-abandoned
    // project are renamed to random alphanumeric strings. This is a fairly
    // dirty fix but it works just fine in the absence of my will to spend
    // time trying to fix this behaviour at the ImGui level.
    // This function should be called before the last render call to the ImGui
    // tabbed tabled before project loading. Hence, it is a separate function 
    // and not embedded in loadState
    void preLoadAdjustment();

    // Save object state to a JSON writer object
    void saveState(ObjectIO& writer);

    // Load object state from a JSON reader object
    void loadState(const ObjectIO& reader);

    // Accessors

    std::vector<Layer*>& layersRef(){return layers_;}
    Layer*& activeGuiLayer(){return activeGuiLayer_;}
    void setActiveGuiLayer(Layer* layer){activeGuiLayer_ = layer;}
    void setTargetResolution(const glm::ivec2& resolution);

};

}

#endif
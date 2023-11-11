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

#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "misc/misc.h"
#include "objectio/objectio.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
//- Static members -----------------------------------------------------------//

unsigned int LayerManager::nLayersSinceNewProject_ = 0;

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

// Constructor/Destructor ----------------------------------------------------//

LayerManager::LayerManager(ShaderThingApp& app):
app_(app),
activeGuiLayer_(nullptr)
{}

LayerManager::~LayerManager()
{
    while (layers_.size() > 0)
    {
        delete layers_[0];
        layers_.erase(layers_.begin());
    }
}

//----------------------------------------------------------------------------//

void LayerManager::renderLayers
(
    vir::Framebuffer* target, 
    unsigned int nRenderPasses
)
{
    auto oneRenderPass = [&](bool clearTarget)
    {
        for (auto layer : layers_)
        {
            layer->render(target, clearTarget);
            //if (lastPass)
            //    layer->render2(target, clearTarget);
            // At the end of the pass, the status of clearTarget will represent
            // whether the main window has been cleared of its contents at least
            // once (true if never cleared at least once)
            if 
            (
                clearTarget &&
                layer->rendersTo() != Layer::RendersTo::InternalFramebuffer
            )
                clearTarget = false;
        }
        // If the window has not been cleared at least once, or if I am not
        // rendering to the window at all (i.e., if target == nullptr, which is
        // only true during exports), then render a dummy/void/blank window,
        // simply to avoid visual artifacts when nothing is rendering to the
        // main window
        if (clearTarget || target != nullptr)
            Layer::renderBlankWindow();
    };

    // Apply internal framebuffer clear policy if exporting, before rendering
    if (app_.isExporting())
    {
        app_.renderPassRef()=0; // Needs to be set before clearing framebuffers
        for (auto layer : layers_)
            layer->clearFramebuffersWithPolicy();
    }

    // Render all layers
    for (int i=0; i<nRenderPasses; i++)
    {
        app_.renderPassRef() = i;
        oneRenderPass(true);
    }
}

//----------------------------------------------------------------------------//

void LayerManager::removeResourceFromUniforms(Resource* resource)
{
    for (auto layer : layers_)
        layer->removeResourceFromUniforms(resource);
}

//----------------------------------------------------------------------------//

void LayerManager::reset()
{
    while (layers_.size() > 0)
    {
        delete layers_[0];
        layers_.erase(layers_.begin());
    }
    nLayersSinceNewProject_ = 0;
}

//----------------------------------------------------------------------------//

void LayerManager::markAllShadersForCompilation()
{
    for (auto* layer : layers_)
        layer->markForCompilation();
}

//----------------------------------------------------------------------------//

void LayerManager::clearFramebuffers()
{
    for (auto* layer : layers_)
        layer->clearFramebuffers();
}

//----------------------------------------------------------------------------//

bool LayerManager::update()
{
    uint32_t i = 0;
    auto i0 = layers_.begin();
    bool someLayersToBeCompiled(false);
    while (i < layers_.size())
    {
        Layer* layer = layers_[i];
        if (layer->toBeDeleted())
        {
            delete layer;
            layers_.erase(i0+i);
            // This is impossible by (current) design, as the user cannot
            // delete the currently rendered layer. Still, just in case I 
            // ever change the approach, I'm adding this
            if (layer == activeGuiLayer_)
            {
                activeGuiLayer_ = nullptr;
                if (i > 0)
                    activeGuiLayer_ = layers_[i-1];
                else if (i < layers_.size()-1)
                    activeGuiLayer_ = layers_[i+1];
            }
        }
        else
        {
            layer->update();
            someLayersToBeCompiled = 
                someLayersToBeCompiled || layer->toBeCompiled();
            i++;
        }
    }
    return someLayersToBeCompiled;
}

//----------------------------------------------------------------------------//

void LayerManager::saveState(ObjectIO& writer)
{
    writer.writeObjectStart("layers");
    for (auto layer : layers_)
        layer->saveState(writer);
    writer.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void LayerManager::loadState(const ObjectIO& reader)
{
    // Clear layers
    reset();

    // Read shared fragment shader code if defined
    auto shared = reader.readObject("shared");
    if (shared.hasMember("sharedFragmentSource"))
        Layer::setSharedSource(shared.read("sharedFragmentSource", false));

    // Read layers
    auto layers = reader.readObject("layers");
    for (auto layerName : layers.members())
    {   
        auto layer = layers.readObject(layerName);
        layers_.emplace_back(new Layer(app_, layer, layers_.size()==0));
    };

    // Re-establish dependencies between Layers (i.e., when a layer is
    // being used as a sampler2D uniform by another layer)
    for (auto* layer : layers_)
    {
        layer->rebindLayerUniforms();
    }
}

//----------------------------------------------------------------------------//

void LayerManager::preLoadAdjustment()
{
    // Because of ImGui stuff I do not grasp, if the project one is loading
    // has a certain number of layers with a certain set of names that
    // are equal to the currently opened project layer names, the order of the 
    // newly loaded project's layer tabs (and thus the order of their rendering)
    // will be randomly reshuffled.
    // To avoid such a situation, the existing layers of the to-be-abandoned
    // project are renamed to random alphanumeric strings. This is a fairly
    // dirty fix but it works just fine in the absence of my will to spend
    // time trying to fix this behaviour at the ImGui level
    for (auto layer : layers_)
        layer->setName(Misc::randomString(9));
}

//----------------------------------------------------------------------------//

void LayerManager::setTargetResolution(const glm::ivec2& resolution)
{
    for(auto layer : layers_)
        layer->setTargetResolution(resolution);
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

Layer* LayerManager::addLayer()
{
    int nLayers = layers_.size();
    float depth = nLayers == 0 ? 0.f : layers_[nLayers-1]->depth()+1e-3;
    auto window = vir::GlobalPtr<vir::Window>::instance();
    auto layer = new Layer
    (
        app_,
        app_.resolutionRef(),
        depth
    );
    std::string name
    (
        "Layer"+std::to_string(LayerManager::nLayersSinceNewProject_)
    );
    Misc::enforceUniqueName(name, layers_);
    layer->setName(name);
    layers_.emplace_back(layer);
    activeGuiLayer_ = layer;
    return layer;
}

}
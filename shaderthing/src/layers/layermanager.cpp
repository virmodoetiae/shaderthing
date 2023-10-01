#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "misc/misc.h"

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
    
    // Actual rendering. The nRenderPasses is per-frame
    for (int i=0; i<nRenderPasses; i++)
        oneRenderPass(true);
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

void LayerManager::saveState(std::ofstream& file)
{
    auto sharedSource = Layer::sharedSource();
    file << sharedSource.size() << std::endl;
    file << sharedSource << std::endl;
    file << layers_.size() << std::endl;
    for (auto layer : layers_)
        layer->saveState(file);
}

//----------------------------------------------------------------------------//

void LayerManager::loadState(std::string& source, uint32_t& index)
{
    // Clear layers
    reset();

    // Read number of shared editor characters to be loaded
    int sharedSourceSize;
    std::string sharedSourceSizeStr = "";
    while(true)
    {
        char& c = source[index];
        if (c=='\n')
            break;
        sharedSourceSizeStr+=c;
        ++index;
    }
    ++index;
    sscanf(sharedSourceSizeStr.c_str(), "%d", &sharedSourceSize);
    
    // Read shared editor source and set it
    int index0(index);
    auto sharedSource = std::string(sharedSourceSize, ' ');
    while (index < index0 + sharedSourceSize)
    {
        sharedSource[index-index0] = source[index];
        ++index;
    }
    ++index;
    Layer::setSharedSource(sharedSource);

    // Read number of layers to be loaded
    int nLayers;
    std::string nLayersStr = "";
    while(true)
    {
        char& c = source[index];
        if (c=='\n')
            break;
        nLayersStr+=c;
        ++index;
    }
    ++index;
    sscanf(nLayersStr.c_str(), "%d", &nLayers);
    
    // Load layers
    while(layers_.size() < nLayers)
    {
        auto layer = new Layer
        (
            app_,
            source,
            index,
            layers_.size() == 0
        );
        layers_.push_back(layer);
    }

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
    // are equal to the currently opened project, the order of the newly loaded 
    // project's layer tabs
    // (and thus the order of their rendering) will be randomly reshuffled.
    // To avoid such a situation, the exiting layers of the to-be-abandoned
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
#include "shaderthing-p/include/modules/backend.h"
#include "shaderthing-p/include/data/layer.h"

#include "vir/include/vir.h"

#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

namespace Backend
{

void addNewLayerTo(std::vector<Layer*>& layers)
{
    // Create new layer with smallest available free id
    auto findFreeId = [](const std::vector<Layer*>& layers)
    {
        std::vector<unsigned int> ids(layers.size());
        unsigned int id(0);
        for (auto l : layers)
            ids[id++] = l->id;
        std::sort(ids.begin(), ids.end());
        for (id=0; id<layers.size(); id++)
        {
            if (id < ids[id])
                return id;
        }
        return id;
    };
    auto layer = new Layer{findFreeId(layers)};
    layers.emplace_back(layer);

    // Set layer detph
    layer->depth = layers.size();

    // Set layer resolution from current app window resolution
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    layer->resolution = {window->width(), window->height()};
    layer->aspectRatio = (float)(layer->resolution.x)/layer->resolution.y;

    // Add default uniforms
    auto addUniformsTo = [](Layer* layer)
    {
        Uniform* u = nullptr;
        
        u = new Uniform();
        u->specialType = Uniform::SpecialType::LayerAspectRatio;
        u->name = "iAspectRatio";
        u->type = Uniform::Type::Float;
        u->setValuePtr(&layer->aspectRatio);
        u->gui.showBounds = false;
        layer->uniforms.emplace_back(u);

        u = new Uniform();
        u->specialType = Uniform::SpecialType::LayerResolution;
        u->name = "iResolution";
        u->type = Uniform::Type::Float2;
        u->setValuePtr(&layer->resolution);
        u->gui.bounds = glm::vec2(1.0f, 4096.0f);
        u->gui.showBounds = false;
        layer->uniforms.emplace_back(u);
    };
    addUniformsTo(layer);

    // Set name
    layer->gui.name = "Layer "+std::to_string(layer->id);
}

void deleteLayerFrom(Layer* layer, std::vector<Layer*>& layers)
{
    layers.erase
    (
        std::remove_if
        (
            layers.begin(), 
            layers.end(), 
            [&layer](const Layer* l) {return l==layer;}
        ), 
        layers.end()
    );
    delete layer;
}

void constrainAndSetWindowResolution(glm::ivec2& resolution)
{
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    auto monitorScale = window->contentScale();
    glm::ivec2 minResolution = {120*monitorScale.x, 1};
    glm::ivec2 maxResolution = window->primaryMonitorResolution();
    resolution.x = 
        std::max(std::min(resolution.x, maxResolution.x), minResolution.x);
    resolution.y = 
        std::max(std::min(resolution.y, maxResolution.y), minResolution.y);
    if (window->width() != resolution.x || window->height() != resolution.y)
        window->setSize
        (
            resolution.x,
            resolution.y
        );
}

}

}
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

    void clearFramebuffers();
    
    void reset();

    bool update();

    void renderGui();

    void saveState(std::ofstream& file);
    void loadState(std::string& source, uint32_t& index);
    void preLoadAdjustment();

    std::vector<Layer*>& layersRef(){return layers_;}

    Layer*& activeGuiLayer(){return activeGuiLayer_;}
    void setActiveGuiLayer(Layer* layer){activeGuiLayer_ = layer;}
    void setTargetResolution(const glm::ivec2& resolution);

};

}

#endif
#ifndef ST_QUANTIZER_TOOL_H
#define ST_QUANTIZER_TOOL_H

#include <unordered_map>

#include "vir/include/vir.h"

namespace ShaderThing
{

class ShaderThingApp;
class Layer;

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
    float clusteringTolerance_;
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
    void saveState(std::ofstream&);
    
    void quantize(Layer* layer);
    void update();
    void reset();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
    Layer*& targetLayer() {return targetLayer_;}
};

}

#endif
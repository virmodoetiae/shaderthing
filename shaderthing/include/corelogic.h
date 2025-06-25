#pragma once

#include <string>
#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

struct AppData;
struct Layer;
struct SharedUniforms;

std::string assembleFragmentShaderHeader
(
    const Layer& layer, 
    const AppData& appData
);
std::string assembleVertexShaderSource(const AppData& appData);
bool compileShader
(
    Layer& layer, 
    AppData& appData, 
    bool setBlankShaderOnError = false
);
void createNewLayer(AppData& appData, bool compileShader = true);
void initialize(AppData& appData);
void initializeSharedUniforms(AppData& appData);
void renderShaders(AppData& appData);
void setupNewProject(AppData& appData);
void setLayerDepth(Layer& layer, const float depth);

}
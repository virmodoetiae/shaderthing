#pragma once

#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

struct AppData;
struct Layer;

void createNewLayer(AppData& appData);
void initialize(AppData& appData);
void renderShaders(AppData& appData);
void setupNewProject(AppData& appData);
void setLayerDepth(Layer& layer, const float depth);

}
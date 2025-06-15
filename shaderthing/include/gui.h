#pragma once

#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

struct AppData;

namespace GUI
{

void renderControlPanel
(
    AppData& appData
);
void renderLayerMenu
(
    Layer& layer, 
    AppData& appData
);
void renderLayerTabBar
(
    Layer& layer, 
    AppData& appData
);
void renderLayersTabBar
(
    AppData& appData
);
void renderMenuBar
(
    AppData& appData
);

}

}
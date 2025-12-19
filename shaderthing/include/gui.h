#pragma once

#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

struct AppData;
class Uniform;

namespace GUI
{

void renderControlPanel
(
    AppData& appData
);

void renderMenuBar
(
    AppData& appData
);

void renderLayerMenu
(
    Layer& layer, 
    AppData& appData
);

void renderLayersTabBar
(
    AppData& appData
);

void renderLayerTab
(
    Layer& layer, 
    AppData& appData
);

/*
void renderUniformsTab
(
    Layer& layer, 
    AppData& appData
);
*/

/*
// Render the button for editing uniform bounds, returns a flag indicating 
// whether the bounds have been changed during this frame
bool renderEditUniformBoundsButton
(
    Uniform& uniform,
    bool renderDragStepSlider = false,
    bool renderLogarithmicZeroSlider = false
);

// Render the default/built-in shared uniforms only as a table and return the
// row count
int renderBuiltInSharedUniformsGui
(
    appData& appData
);
*/

}

}
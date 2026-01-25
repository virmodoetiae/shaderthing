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
    UPtr<Layer>& layer, 
    AppData& appData
);

void renderLayersTabBar
(
    AppData& appData
);

void renderLayerTab
(
    UPtr<Layer>& layer, 
    AppData& appData
);

void renderUniformsTab
(
    UPtr<Layer>& layer, 
    AppData& appData
);

// Render the default/built-in shared uniforms only as a table and return the
// row count
int renderBuiltInSharedUniforms
(
    AppData& appData
);


// Render the button for editing uniform bounds, returns a flag indicating 
// whether the bounds have been changed during this frame
bool renderEditUniformBoundsButton
(
    UPtr<Uniform>& uniform,
    bool renderDragStepSlider = false
);

//
bool renderUniformGui
(
    UPtr<Uniform>& uniform,
    UPtr<Layer>& layer,
    AppData& appData,
    int& row,
    const bool showSeparator = false,
    const bool showSharedAndDefaultUniforms = true
);

}

}
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

void renderLayerMenuItem
(
    UPtr<Layer>& layer, 
    AppData& appData
);

void renderLayerFramebufferSettings
(
    Layer* layer,
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
    const vir::Ptr<Uniform>& uniform,
    bool renderDragStepSlider = false
);

// Render the GUI of the provided uniform at the provided table row and return
// true if the uniform type is changed by user interation with this GUI, else
// false
bool renderUniformTableRow
(
    const UPtr<Uniform>& uniform,
    const UPtr<Layer>& layer,
    AppData& appData,
    int row,
    const bool showSeparator = false,
    const bool showSharedAndDefaultUniforms = true
);

//
void renderResourcesMenuItem(AppData& appData);

//
void renderResourcesTable(AppData& appData);

//
void renderResourcesTableRow(AppData& appData, int row);

//
void renderAddResourceButton(AppData& appData, int row);

//
void renderResourceActionsButton(AppData& appData, int row);

// Overloads for convenience ---------------------------------------------------

//
inline void renderLayerFramebufferSettings
(
    const WPtr<Layer>& layer,
    AppData& appData
)
{
    renderLayerFramebufferSettings(layer.get(), appData);
}

//
inline void renderLayerFramebufferSettings
(
    const UPtr<Layer>& layer,
    AppData& appData
)
{
    renderLayerFramebufferSettings(layer.get(), appData);
}

}

}
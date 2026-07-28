#pragma once

#include "shaderthing/include/typedefs.h"

namespace ShaderThing
{

struct AppState;
class Uniform;

namespace GUI
{

// gui_main.cpp
void renderControlPanel
(
    AppState& appState
);

// gui_main.cpp
void renderMenuBar
(
    AppState& appState
);

// gui_layer.cpp
void renderLayersTabBar
(
    AppState& appState
);

// gui_uniform.cpp
void renderUniformsTab
(
    Layer* layer, 
    AppState& appState
);

// Render the default/built-in shared uniforms only as a table and return the
// row count
// gui_uniform.cpp
int renderBuiltInSharedUniforms
(
    AppState& appState
);

// Render the button for editing uniform bounds, returns a flag indicating 
// whether the bounds have been changed during this frame
// gui_uniform.cpp
bool renderEditUniformBoundsButton
(
    const vir::Ptr<Uniform>& uniform,
    bool renderDragStepSlider = false
);

// Render the GUI of the provided uniform at the provided table row and return
// true if the uniform type is changed by user interation with this GUI, else
// false
// gui_uniform.cpp
bool renderUniformTableRow
(
    UPtr<Uniform>& uniform,
    Layer* layer,
    AppState& appState,
    int row,
    const bool showSeparator = false,
    const bool showSharedAndDefaultUniforms = true
);

// gui_resource.cpp
void renderResourcesMenuItem(AppState& appState);

// gui_resource.cpp
void renderResourcesTable(AppState& appState);

// gui_resource.cpp
void renderResourcesTableRow(AppState& appState, int row);

// gui_resource.cpp
void renderAddResourceButton(AppState& appState, int row);

// gui_resource.cpp
void renderResourceActionsButton(AppState& appState, int row);

}

}
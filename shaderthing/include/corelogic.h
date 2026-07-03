#pragma once

#include <string>
#include "shaderthing/include/typedefs.h"
#include "shaderthing/include/oo/uniform.h"

namespace ShaderThing
{

struct AppState;
struct Layer;
struct RenderResult;
struct SharedUniforms;

// logic_initialization.cpp
void initialize(AppState& appState);

void preRenderUpdate(AppState& appState);

// logic_rendering.cpp
RenderResult renderShaders
(
    AppState& appState, 
    vir::Framebuffer* target, 
    const unsigned int nRenderPasses
);

void postRenderUpdate(AppState& appState);

void setWindowResolution
(
    AppState& appState, 
    glm::ivec2 resolution, 
    const bool windowFrameManuallyDragged,
    const bool prepareForExport = false
);

// logic_layer.cpp
void createNewLayer(AppState& appState, bool compileShader = true);

// logic_rendering.cpp
void setRenderingTiles(AppState& appState, int nTiles);

// logic_misc.cpp
void addLayerToResources(WPtr<Layer> layer, UPtrVector<Resource>& resources);

// logic_misc.cpp
void removeLayerFromResources
(
    WPtr<Layer> layer, 
    UPtrVector<Resource>& resources
);

// logic_misc.cpp
void toggleRenderingPaused(AppState& appState, bool dueToFlowFps);

// logic_misc.cpp
void toggleKeyboardInputs(AppState& appState);

// logic_misc.cpp
void toggleMouseInputs(AppState& appState);

// logic_misc.cpp
void toggleCameraMouseInputs(AppState& appState);

// logic_misc.cpp
void toggleCameraKeyboardInputs(AppState& appState);

// logic_misc.cpp
void setMouseInputsClamped(AppState& appState, bool flag);

// logic_misc.cpp
void setMouseCaptured(AppState& appState, bool flag);

// logic_saveload.cpp
void saveToDisk
(
    AppState& appState, 
    const std::string& filepath, 
    bool triggeredByAutosave = false
);

}
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

void initialize(AppState& appState);

void initializeSharedUniforms(AppState& appState);

void preRenderUpdate(AppState& appState);

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

void createNewLayer(AppState& appState, bool compileShader = true);

void setRenderingTiles(AppState& appState, int nTiles);

void addLayerToResources(WPtr<Layer> layer, UPtrVector<Resource>& resources);

void removeLayerFromResources
(
    WPtr<Layer> layer, 
    UPtrVector<Resource>& resources
);

void toggleRenderingPaused(AppState& appState, bool dueToFlowFps);

void toggleKeyboardInputs(AppState& appState);

void toggleMouseInputs(AppState& appState);

void toggleCameraMouseInputs(AppState& appState);

void toggleCameraKeyboardInputs(AppState& appState);

void setMouseInputsClamped(AppState& appState, bool flag);

void setMouseCaptured(AppState& appState, bool flag);

void saveTo
(
    AppState& appState, 
    const std::string& filepath, 
    bool triggeredByAutosave = false
);

//
void loadFrom
(
    AppState& appState,
    const std::string& filepathOrData,
    bool fromMemory
);

void updateLayersDueToUniformTypeOrNameChanged
(
    UPtr<Uniform>& uniform, 
    AppState& appState,
    bool recompileShaders = false
);

void updateLayersDueToUniformDeletion
(
    UPtr<Uniform>& uniform, 
    AppState& appState
);

}
#pragma once

#include <string>
#include "shaderthing/include/typedefs.h"
#include "shaderthing/include/oo/uniform.h"

namespace ShaderThing
{

struct AppData;
struct Layer;
struct RenderResult;
struct SharedUniforms;

void initialize(AppData& appData);
void initializeSharedUniforms(AppData& appData);

void setupNewProject(AppData& appData);

void preRenderUpdate(AppData& appData);
RenderResult renderShaders
(
    AppData& appData, 
    vir::Framebuffer* target, 
    const unsigned int nRenderPasses
);
void postRenderUpdate(AppData& appData);

void setWindowResolution
(
    AppData& appData, 
    glm::ivec2 resolution, 
    const bool windowFrameManuallyDragged,
    const bool prepareForExport = false
);

void createNewLayer(AppData& appData, bool compileShader = true);
void setLayerDepth(UPtr<Layer>& layer, const float depth);
void setLayerFramebufferWrapMode(UPtr<Layer>& layer, int i, WrapMode mode);
void setLayerFramebufferMagFilterMode(UPtr<Layer>& layer, FilterMode mode);
void setLayerFramebufferMinFilterMode(UPtr<Layer>& layer, FilterMode mode);
void setLayerResolution
(
    UPtr<Layer>& layer,
    glm::ivec2 resolution,
    const bool isTiledRenderingEnabled,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio = false,
    const bool setExportResolution = true
);
void rebuildLayerFramebuffers
(
    UPtr<Layer>& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const AppData& appData
);
void clearLayerFramebuffers(Layer& layer);
std::string assembleFragmentShaderHeader
(
    const UPtr<Layer>& layer, 
    const AppData& appData
);
std::string assembleVertexShaderSource(const AppData& appData);
bool compileShader
(
    UPtr<Layer>& layer, 
    AppData& appData, 
    bool setBlankShaderOnError = false
);
void renderLayerShader
(
    UPtr<Layer>& layer,
    vir::Framebuffer* target,
    const bool clearTarget,
    AppData& appData
);

void addUniformToLayer(UPtr<Uniform>&& uniform, UPtr<Layer>& layer);

UPtr<Uniform> removeUniformFromLayer
(
    UPtr<Uniform>& uniform, 
    UPtr<Layer>& layer
);

void addUniformToSharedUniforms(UPtr<Uniform>&& uniform, AppData& appData);

UPtr<Uniform> removeUniformFromSharedUniforms
(
    UPtr<Uniform>& uniform, 
    AppData& appData
);

void toggleRenderingPaused(AppData& appData, bool dueToFlowFps);

void toggleKeyboardInputs(AppData& appData);

void toggleMouseInputs(AppData& appData);

void toggleCameraMouseInputs(AppData& appData);

void toggleCameraKeyboardInputs(AppData& appData);

void setMouseInputsClamped(AppData& appData, bool flag);

}
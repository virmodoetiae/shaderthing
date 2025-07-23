#pragma once

#include <string>
#include "shaderthing/include/typedefs.h"

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
RenderResult renderShaders(AppData& appData);
void postRenderUpdate(AppData& appData);

void createNewLayer(AppData& appData, bool compileShader = true);
void setLayerDepth(Layer& layer, const float depth);
void setLayerFramebufferWrapMode(Layer& layer, int i, WrapMode mode);
void setLayerFramebufferMagFilterMode(Layer& layer, FilterMode mode);
void setLayerFramebufferMinFilterMode(Layer& layer, FilterMode mode);
void setLayerResolution
(
    Layer& layer,
    glm::ivec2 resolution,
    const bool isTiledRenderingEnabled,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio = false,
    const bool setExportResolution = true
);
void rebuildLayerFramebuffers
(
    Layer& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const AppData& appData
);
void clearLayerFramebuffers(Layer& layer);
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

}
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

void setLayerResolution
(
    UPtr<Layer>& layer,
    glm::ivec2 resolution,
    const bool isTiledRenderingEnabled,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio = false,
    const bool setExportResolution = true
);

void setLayerDepth(UPtr<Layer>& layer, const float depth);

void setLayerFramebufferWrapMode(Layer* layer, int i, WrapMode mode);

void setLayerFramebufferMagFilterMode(Layer* layer, FilterMode mode);

void setLayerFramebufferMinFilterMode(Layer* layer, FilterMode mode);

void rebuildLayerFramebuffers
(
    Layer* layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const bool isTiledRenderingEnabled
);

void clearLayerFramebuffers(UPtr<Layer>& layer);

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

void setRenderingTiles(AppData& appData, int nTiles);

void renderLayerShader
(
    UPtr<Layer>& layer,
    vir::Framebuffer* target,
    const bool clearTarget,
    AppData& appData
);

void addLayerToResources(UPtr<Layer>& layer, UPtrVector<Resource>& resources);

void removeLayerFromResources
(
    UPtr<Layer>& layer, 
    UPtrVector<Resource>& resources
);

void toggleRenderingPaused(AppData& appData, bool dueToFlowFps);

void toggleKeyboardInputs(AppData& appData);

void toggleMouseInputs(AppData& appData);

void toggleCameraMouseInputs(AppData& appData);

void toggleCameraKeyboardInputs(AppData& appData);

void setMouseInputsClamped(AppData& appData, bool flag);

void setMouseCaptured(AppData& appData, bool flag);

// Save project to disk
void saveToDisk
(
    AppData& appData, 
    const std::string& filepath, 
    bool triggeredByAutosave
);

// Save layer data to disk
void saveToDisk(UPtr<Layer>& layer, ObjectIO& io);

// Overloads for convenience ---------------------------------------------------

inline void setLayerFramebufferWrapMode(WPtr<Layer>& layer, int i, WrapMode mode)
{
    setLayerFramebufferWrapMode(layer.get(), i, mode);
}
inline void setLayerFramebufferWrapMode(UPtr<Layer>& layer, int i, WrapMode mode)
{
    setLayerFramebufferWrapMode(layer.get(), i, mode);
}

inline void setLayerFramebufferMagFilterMode(WPtr<Layer>& layer, FilterMode mode)
{
    setLayerFramebufferMagFilterMode(layer.get(), mode);
}
inline void setLayerFramebufferMagFilterMode(UPtr<Layer>& layer, FilterMode mode)
{
    setLayerFramebufferMagFilterMode(layer.get(), mode);
}

inline void setLayerFramebufferMinFilterMode(WPtr<Layer>& layer, FilterMode mode)
{
    setLayerFramebufferMinFilterMode(layer.get(), mode);
}
inline void setLayerFramebufferMinFilterMode(UPtr<Layer>& layer, FilterMode mode)
{
    setLayerFramebufferMinFilterMode(layer.get(), mode);
}

inline void rebuildLayerFramebuffers
(
    WPtr<Layer>& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const bool isTiledRenderingEnabled
)
{
    rebuildLayerFramebuffers
    (
        layer.get(), 
        internalFormat, 
        resolution, 
        isTiledRenderingEnabled
    );
}
inline void rebuildLayerFramebuffers
(
    UPtr<Layer>& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const bool isTiledRenderingEnabled
)
{
    rebuildLayerFramebuffers
    (
        layer.get(), 
        internalFormat, 
        resolution, 
        isTiledRenderingEnabled
    );
}

}
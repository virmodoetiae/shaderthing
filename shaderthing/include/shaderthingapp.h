#ifndef ST_SHADERTHING_APP_H
#define ST_SHADERTHING_APP_H

#include <iomanip>

#include "vir/include/vir.h"

namespace ShaderThing
{

#define ST_NEW_PROJECT 0
#define ST_SAVE_PROJECT 1
#define ST_LOAD_PROJECT 2
#define ST_OPEN_SAVE_DIALOG 3
#define ST_OPEN_LOAD_DIALOG 4
#define ST_IS_TIME_PAUSED 5
#define ST_IS_WINDOW_ICONIFIED 6
#define ST_IS_CAMERA_POSITION_INPUT_ENABLED 7
#define ST_IS_CAMERA_DIRECTION_INPUT_ENABLED 8
#define ST_IS_MOUSE_INPUT_ENABLED 9
#define ST_NEW_PROJECT_CONFIRMATION_PENDING 10
#define ST_IS_RENDERING_PAUSED 11

class Layer;
class LayerManager;
class Resource;
class ResourceManager;
class ExportTool;
class QuantizationTool;
class FindReplaceTextTool;
class CodeRepository;
class About;

class ShaderThingApp : public vir::Event::Receiver
{

private:

    // Generic state flags (see ST_ defines)
    bool stateFlags_[12];

    // Window state
    float time_ = 0.0;
    int frame_ = 0;
    glm::ivec2 resolution_ = {512, 512};
    glm::vec2 viewport_; // i.e., normalized resolution in a 0-1 range
    vir::Camera* screenCamera_ = nullptr;

    // Inputs/default uniforms
    vir::Camera* shaderCamera_ = nullptr;
    glm::ivec4 mouse_ = {0,0,0,0};

    // Filepath of the currently loaded project/shaderthing instance
    std::string projectFilepath_ = "";
    std::string projectFilename_ = "";

    // Components
    LayerManager* layerManager_ = nullptr;
    ResourceManager* resourceManager_ = nullptr;
    QuantizationTool* quantizationTool_ = nullptr;
    ExportTool* exportTool_ = nullptr;
    FindReplaceTextTool* findReplaceTextTool_ = nullptr;
    CodeRepository* codeRepository_ = nullptr;
    About* about_ = nullptr;

    void newProject();
    void saveProject();
    void loadProject();

    void restart();
    void update();
    void updateGui();
    void renderGuiNewProject();
    void renderGuiSaveProject();
    void renderGuiLoadProject();

public:

    ShaderThingApp();
    ~ShaderThingApp();

    //
    void setShaderCameraPositionInputsEnabled(bool status);
    void setShaderCameraDirectionInputsEnabled(bool status);
    void setMouseInputsEnabled(bool status);
    
    // Clears all layer framebuffers, rersets frame_ and time_ to 0
    void restartRendering();

    // Accessors -------------------------------------------------------------//
    float& timeRef(){return time_;}
    bool& isTimePausedRef(){return stateFlags_[ST_IS_TIME_PAUSED];}
    const bool& isTimePausedCRef() const 
    {
        return stateFlags_[ST_IS_TIME_PAUSED];
    }
    bool& isRenderingPausedRef() {return stateFlags_[ST_IS_RENDERING_PAUSED];}
    const bool& isRenderingPausedCRef() const 
    {
        return stateFlags_[ST_IS_RENDERING_PAUSED];
    }
    int& frameRef(){return frame_;}
    glm::ivec2& resolutionRef(){return resolution_;}
    glm::vec2& viewportRef(){return viewport_;}
    vir::Camera& screnCameraRef(){return *screenCamera_;}
    vir::Camera& shaderCameraRef(){return *shaderCamera_;}
    glm::ivec4& mouseRef(){return mouse_;}
    bool isCameraPositionInputEnabled()
    {
        return stateFlags_[ST_IS_CAMERA_POSITION_INPUT_ENABLED];
    }
    bool isCameraDirectionInputEnabled()
    {
        return stateFlags_[ST_IS_CAMERA_DIRECTION_INPUT_ENABLED];
    }
    bool isMouseInputEnabled()
    {
        return stateFlags_[ST_IS_MOUSE_INPUT_ENABLED];
    }
    LayerManager& layerManagerRef(){return *layerManager_;}
    ResourceManager& resourceManagerRef(){return *resourceManager_;}
    std::vector<Layer*>& layersRef();
    std::vector<Resource*>& resourcesRef();
    QuantizationTool& quantizationToolRef() {return *quantizationTool_;}
    ExportTool& exportToolRef() {return *exportTool_;}
    FindReplaceTextTool& findReplaceTextToolRef(){return *findReplaceTextTool_;}

    // Event-related ---------------------------------------------------------//
    DECLARE_RECEIVABLE_EVENTS
    (
        vir::Event::Type::WindowResize *
        vir::Event::Type::WindowIconification *
        vir::Event::Type::WindowMaximization *
        vir::Event::Type::WindowFocus *
        vir::Event::Type::MouseButtonPress *
        vir::Event::Type::MouseMotion *
        vir::Event::Type::MouseButtonRelease
    )
    void onReceive(vir::Event::WindowResizeEvent& e) override;
    void onReceive(vir::Event::WindowIconificationEvent& e) override;
    void onReceive(vir::Event::WindowMaximizationEvent& e) override;
    void onReceive(vir::Event::WindowFocusEvent& e) override;
    void onReceive(vir::Event::MouseButtonPressEvent& e) override;
    void onReceive(vir::Event::MouseMotionEvent& e) override;
    void onReceive(vir::Event::MouseButtonReleaseEvent& e) override;

};

}

#endif
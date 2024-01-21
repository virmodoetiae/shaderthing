/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include <iomanip> // For std::setprecision

#include "shaderthingapp.h"
#include "objectio/objectio.h"
#include "uniforms/uniform.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "resources/resourcemanager.h"
#include "tools/exporttool.h"
#include "tools/quantizationtool.h"
#include "tools/findreplacetexttool.h"
#include "data/coderepository.h"
#include "data/data.h"
#include "data/about.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/rapidjson/include/rapidjson/document.h"
#include "thirdparty/rapidjson/include/rapidjson/writer.h"
#include "thirdparty/rapidjson/include/rapidjson/prettywriter.h"
#include "thirdparty/rapidjson/include/rapidjson/stringbuffer.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

// Constructor/destructor ----------------------------------------------------//

ShaderThingApp::ShaderThingApp() :
sharedUniforms_(0)
{
    // Initialize vir lib
    vir::initialize
    (
        vir::PlatformType::GLFWOpenGL,
        resolution_.x,
        resolution_.y,
        "ShaderThing"
    );

    // Set window icon
    auto window = vir::GlobalPtr<vir::Window>::instance();
    window->setIcon
    (
        (unsigned char*)IconData::sTIconData,
        IconData::sTIconSize,
        false
    );

    // Register the app with the event broadcaster
    tuneIn();
    
    // Initialize components
    screenCamera_ = vir::Camera::create<vir::Camera>();
    shaderCamera_ = vir::Camera::create<vir::InputCamera>();
    layerManager_ = new LayerManager(*this);
    resourceManager_ = new ResourceManager(*this);
    exportTool_ = new ExportTool(*this);
    //quantizationTool_ = new QuantizationTool(*this);
    findReplaceTextTool_ = new FindReplaceTextTool();
    codeRepository_ = new CodeRepository();
    about_ = new About();
    reset();

    // Main loop
    while(window->isOpen())
    {
        // Rendering
        if (!isRenderingPausedCRef())
        {
            vir::Framebuffer* target = nullptr; // nullptr == to window
            uint32_t nRenderPasses = 1;
            if (exportTool_->isExporting())
            {
                target = exportTool_->exportFramebuffer();
                nRenderPasses = exportTool_->nRendersPerFrame();
            }
            layerManager_->renderLayers(target, nRenderPasses);
        }
        updateGui();
        update();
    }
}

ShaderThingApp::~ShaderThingApp()
{
    delete layerManager_;
    delete resourceManager_;
    delete exportTool_;
    //delete quantizationTool_;
    delete codeRepository_;
    delete about_;
    delete keyobardUniformBuffer_;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::setShaderCameraPositionInputsEnabled(bool status)
{
    static auto* iShaderCamera((vir::InputCamera*)shaderCamera_);
    stateFlags_[ST_IS_CAMERA_POSITION_INPUT_ENABLED] = status;
    if (status)
        iShaderCamera->enableCurrentlyReceiving(vir::Event::KeyPress);
    else
        iShaderCamera->disableCurrentlyReceiving(vir::Event::KeyPress);
}

//----------------------------------------------------------------------------//

void ShaderThingApp::setShaderCameraDirectionInputsEnabled(bool status)
{
    static auto* iShaderCamera((vir::InputCamera*)shaderCamera_);
    stateFlags_[ST_IS_CAMERA_DIRECTION_INPUT_ENABLED] = status;
    if (status)
        iShaderCamera->enableCurrentlyReceiving(vir::Event::MouseMotion);
    else
        iShaderCamera->disableCurrentlyReceiving(vir::Event::MouseMotion);
}

//----------------------------------------------------------------------------//

void ShaderThingApp::setMouseInputsEnabled(bool status)
{
    stateFlags_[ST_IS_MOUSE_INPUT_ENABLED] = status;
    if (status)
    {
        this->enableCurrentlyReceiving(vir::Event::MouseButtonPress);
        this->enableCurrentlyReceiving(vir::Event::MouseMotion);
        this->enableCurrentlyReceiving(vir::Event::MouseButtonRelease);
    }
    else
    {
        this->disableCurrentlyReceiving(vir::Event::MouseButtonPress);
        this->disableCurrentlyReceiving(vir::Event::MouseMotion);
        this->disableCurrentlyReceiving(vir::Event::MouseButtonRelease);
    }
}

//----------------------------------------------------------------------------//

void ShaderThingApp::restartRendering(bool restartTime)
{
    frame_ = -1;
    if (stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART] || restartTime)
        time_ = 0.f;
    layerManager_->clearFramebuffers();
}

//----------------------------------------------------------------------------//

std::vector<Layer*>& ShaderThingApp::layersRef()
{
    return layerManager_->layersRef();
}

//----------------------------------------------------------------------------//

std::vector<Resource*>& ShaderThingApp::resourcesRef()
{
    return resourceManager_->resourcesRef();
}

//----------------------------------------------------------------------------//

bool ShaderThingApp::isExporting()
{
    return exportTool_->isExporting();
}

//----------------------------------------------------------------------------//

bool ShaderThingApp::isExportingAndFirstFrame()
{
    return exportTool_->isExporting() && exportTool_->frame() <= 1;
}

//----------------------------------------------------------------------------//

bool ShaderThingApp::isExportingAndFirstRenderPassInFrame()
{
    return exportTool_->isExporting() && renderPass_ == 0;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::WindowResizeEvent& e)
{
    if (stateFlags_[ST_IS_WINDOW_ICONIFIED])
        return;

    const glm::ivec2 resolution({e.width(), e.height()});
    
    if (resolution_ == resolution)
        return;
    resolution_ = resolution;
    layerManager_->setTargetResolution(resolution);
    exportTool_->setExportResolution(resolution);

    float aspectRatio = float(resolution.x)/float(resolution.y);
    viewport_.x = std::min(1.0f, aspectRatio);
    viewport_.y = std::min(1.0f, 1.0f/aspectRatio);
    screenCamera_->setViewportHeight(viewport_.y);
    screenCamera_->update();

    // Lightweight version of render restart
    frame_ = -1;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::WindowIconificationEvent& event)
{
    (void)event;
    stateFlags_[ST_IS_WINDOW_ICONIFIED] = true;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::WindowMaximizationEvent& event)
{
    (void)event;
    stateFlags_[ST_IS_WINDOW_ICONIFIED] = false;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::WindowFocusEvent& event)
{
    (void)event;
    stateFlags_[ST_IS_WINDOW_ICONIFIED] = false;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::MouseButtonPressEvent& event)
{
    mouse_.x = event.x();
    mouse_.y = resolution_.y-event.y();
    mouse_.z = mouse_.x;
    mouse_.w = -mouse_.y;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::MouseMotionEvent& event)
{
    if 
    (
        vir::GlobalPtr<vir::InputState>::instance()->
        mouseButtonState(VIR_MOUSE_BUTTON_1).isClicked()
    )
    {
        mouse_.x = event.x();
        mouse_.y = resolution_.y-event.y();
    }
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::MouseButtonReleaseEvent& event)
{
    mouse_.z *= -1;
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::KeyPressEvent& event)
{
    KeyboardUniformData::ivec3A16& data
    (
        keyboardUniformData_.data[event.keyCode()]
    );
    static uint32_t size = sizeof(data);
    static auto* inputState = vir::GlobalPtr<vir::InputState>::instance();
    int shaderToyKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode());
    auto& status = inputState->keyState(event.keyCode());
    data.x = (int)status.isPressed();
    data.y = (int)status.isHeld();
    data.z = (int)status.isToggled();
    //                                  key data,   16,    key pos. in buffer
    keyobardUniformBuffer_->setData((void*)&data, size, size*shaderToyKeyCode);
}

//----------------------------------------------------------------------------//

void ShaderThingApp::onReceive(vir::Event::KeyReleaseEvent& event)
{
    KeyboardUniformData::ivec3A16& data
    (
        keyboardUniformData_.data[event.keyCode()]
    );
    static uint32_t size = sizeof(data);
    static auto* inputState = vir::GlobalPtr<vir::InputState>::instance();
    int shaderToyKeyCode = vir::inputKeyCodeVirToShaderToy(event.keyCode());
    data.x = 0;
    data.y = 0;
    data.z = (int)inputState->keyState(event.keyCode()).isToggled();
    //                                  key data,   16,    key pos. in buffer
    keyobardUniformBuffer_->setData((void*)&data, size, size*shaderToyKeyCode);
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

void ShaderThingApp::reset()
{
    // Reset app
    time_ = 0.0;
    frame_ = 0;
    mouse_ = glm::ivec4(0,0,0,0);
    resolution_ = glm::ivec2(512,512);
    projectFilepath_ = "";
    projectFilename_ = "";
    float aspectRatio = float(resolution_.x)/float(resolution_.y);
    viewport_.x = std::min(1.0f, aspectRatio);
    viewport_.y = std::min(1.0f, 1.0f/aspectRatio);
    screenCamera_->setProjectionType(vir::Camera::ProjectionType::Orthographic);
    screenCamera_->setViewportHeight(viewport_.y);
    auto cameraPosition = glm::vec3(0,0,1);
    screenCamera_->setPosition(cameraPosition);
    cameraPosition = glm::vec3(0,0,-1);
    shaderCamera_->setPosition(cameraPosition);
    shaderCamera_->setZPlusIsLookDirection(true);
    for (int i=0; i<ST_N_STATE_FLAGS; i++)
        stateFlags_[i] = (i>6 && i<10);
    stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART] = true;
    // Restart shared uniforms
    resetSharedUniforms();
    // Restart app components
    layerManager_->reset();
    resourceManager_->reset();
    exportTool_->reset();
    //quantizationTool_->reset();
    exportTool_->reset();

    static bool startup(true);
    if (startup)
    {
        // Set renderer options
        auto renderer = vir::GlobalPtr<vir::Renderer>::instance();
        renderer->setBlending(true);
        renderer->setFaceCulling(false);
        startup = false;
    }
    else
    {
        auto window = vir::GlobalPtr<vir::Window>::instance();
        window->setSize(resolution_.x, resolution_.y);
    }
}

void ShaderThingApp::newProject()
{
    if (!stateFlags_[ST_NEW_PROJECT]) return;
    stateFlags_[ST_NEW_PROJECT] = false;
    reset();
}

//----------------------------------------------------------------------------//

void ShaderThingApp::resetSharedUniforms()
{
    // Initial keyboard UBO data to all 0s. A note for future me, I chose to
    // support only 256 keys, and each array element needs to have a padding of
    // 16 because of the std140 layout, hence the kUBOSize value
    static constexpr uint32_t kUBOSize = 256*16;
    if (keyobardUniformBuffer_ == nullptr);
        keyobardUniformBuffer_ = vir::UniformBuffer::create(kUBOSize);
    auto keyboardData0 = new int[kUBOSize];
    std::memset(keyboardData0, 0, kUBOSize);
    keyobardUniformBuffer_->bind(0);
    keyobardUniformBuffer_->setData(keyboardData0);
    delete[] keyboardData0;
    
    // Clear all shared uniforms
    for (auto* u : sharedUniforms_)
        delete u;
    sharedUniforms_.resize(0);

    // iFrame
    auto frameUniform = new Uniform();
    frameUniform->specialType = Uniform::SpecialType::Frame;
    frameUniform->name = "iFrame";
    frameUniform->type = Uniform::Type::UInt;
    frameUniform->setValuePtr(&frame_);
    frameUniform->isShared = true;
    frameUniform->showLimits = false;
    sharedUniforms_.emplace_back(frameUniform);

    // iRenderPass
    auto renderPassUniform = new Uniform();
    renderPassUniform->specialType = Uniform::SpecialType::RenderPass;
    renderPassUniform->name = "iRenderPass";
    renderPassUniform->type = Uniform::Type::UInt;
    renderPassUniform->setValuePtr(&renderPass_);
    renderPassUniform->isShared = true;
    renderPassUniform->showLimits = false;
    sharedUniforms_.emplace_back(renderPassUniform);

    // iTime
    auto timeUniform = new Uniform();
    timeUniform->specialType = Uniform::SpecialType::Time;
    timeUniform->name = "iTime";
    timeUniform->type = Uniform::Type::Float;
    timeUniform->setValuePtr(&time_);
    sharedUniforms_.emplace_back(timeUniform);
    timeLoopBounds_ = &(timeUniform->limits);

    // iAspectRatio
    auto aspectRatio = new Uniform();
    aspectRatio->specialType = Uniform::SpecialType::WindowAspectRatio;
    aspectRatio->name = "iWindowAspectRatio";
    aspectRatio->type =  Uniform::Type::Float;
    aspectRatio->isShared = true;
    aspectRatio->setValuePtr
    (
        &vir::GlobalPtr<vir::Window>::instance()->aspectRatio()
    );
    aspectRatio->showLimits = false;
    sharedUniforms_.emplace_back(aspectRatio);

    // iResolution
    auto resolutionUniform = new Uniform();
    resolutionUniform->specialType = Uniform::SpecialType::WindowResolution;
    resolutionUniform->name = "iWindowResolution";
    resolutionUniform->type = Uniform::Type::Float2;
    resolutionUniform->setValuePtr(&resolution_);
    resolutionUniform->isShared = true;
    resolutionUniform->limits = glm::vec2(1.0f, 4096.0f);
    resolutionUniform->showLimits = false;
    sharedUniforms_.emplace_back(resolutionUniform);
    
    // Keyboard (for GUI purposes only)
    auto keyboardUniform = new Uniform();
    keyboardUniform->specialType = Uniform::SpecialType::Keyboard;
    keyboardUniform->name = "iKeyboard";
    keyboardUniform->type = Uniform::Type::Int3;
    keyboardUniform->showLimits = false;
    sharedUniforms_.emplace_back(keyboardUniform);

    // iMouse
    auto mouseUniform = new Uniform();
    mouseUniform->specialType = Uniform::SpecialType::Mouse;
    mouseUniform->name = "iMouse";
    mouseUniform->type = Uniform::Type::Int4;
    mouseUniform->setValuePtr(&mouse_);
    mouseUniform->limits = glm::vec2(-1.f, 4096.f);
    mouseUniform->showLimits = false;
    sharedUniforms_.emplace_back(mouseUniform);

    // Shader camera position
    auto cameraPositionUniform = new Uniform();
    cameraPositionUniform->specialType = Uniform::SpecialType::CameraPosition;
    cameraPositionUniform->name = "iWASD";
    cameraPositionUniform->type = Uniform::Type::Float3;
    cameraPositionUniform->setValuePtr(&(shaderCamera_->position()));
    cameraPositionUniform->limits = glm::vec2(-1.0f, 1.0f);
    sharedUniforms_.emplace_back(cameraPositionUniform);

    // Shader camera direction
    auto cameraDirectionUniform = new Uniform();
    cameraDirectionUniform->specialType = Uniform::SpecialType::CameraDirection;
    cameraDirectionUniform->name = "iLook";
    cameraDirectionUniform->type = vir::Shader::Variable::Type::Float3;
    cameraDirectionUniform->setValuePtr(&(shaderCamera_->z()));
    cameraDirectionUniform->limits = glm::vec2(-1.0f, 1.0f);
    cameraDirectionUniform->showLimits = false;
    sharedUniforms_.emplace_back(cameraDirectionUniform);
}

//----------------------------------------------------------------------------//

void ShaderThingApp::saveProject(){
    if (!stateFlags_[ST_SAVE_PROJECT]) return;
    stateFlags_[ST_SAVE_PROJECT] = false;

    if (projectFilepath_.size() == 0)
        return;

    auto project = ObjectIO(projectFilepath_.c_str(), ObjectIO::Mode::Write);
    
    project.writeObjectStart("shared");
    project.write("windowResolution", resolution_);
    project.write("time", time_);
    project.write("timePaused", stateFlags_[ST_IS_TIME_PAUSED]);
    project.write("timeLooped", stateFlags_[ST_IS_TIME_LOOPED]);
    project.write("iWASD", shaderCamera_->position());
    project.write("iWASDSensitivity", shaderCamera_->keySensitivityRef());
    project.write
    (
        "iWASDInputEnabled", 
        stateFlags_[ST_IS_CAMERA_POSITION_INPUT_ENABLED]
    );
    project.write("iLook", shaderCamera_->z());
    project.write("iLookSensitivity",shaderCamera_->mouseSensitivityRef());
    project.write
    (
        "iLookInputEnabled",
        stateFlags_[ST_IS_CAMERA_DIRECTION_INPUT_ENABLED]
    );
    project.write
    (
        "iMouseInputEnabled", 
        stateFlags_[ST_IS_MOUSE_INPUT_ENABLED]
    );
    project.write
    (
        "resetTimeOnRenderRestart",
        stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART]
    );
    project.write("UIScale", *fontScale_);
    if (Layer::sharedSourceIsNotDefault())
    {
        auto sharedSource = Layer::sharedSource();
        project.write("sharedFragmentSource", sharedSource.c_str(), 
            sharedSource.size(), true);
    };
    project.writeObjectEnd();

    resourceManager_->saveState(project);
    layerManager_->saveState(project);
    //quantizationTool_->saveState(project);
    exportTool_->saveState(project);
}

//----------------------------------------------------------------------------//

void ShaderThingApp::loadProject()
{
    if (!stateFlags_[ST_LOAD_PROJECT]) return;
        stateFlags_[ST_LOAD_PROJECT] = false;

    if (projectFilepath_.size() == 0)
        return;

    auto project = ObjectIO(projectFilepath_.c_str(), ObjectIO::Mode::Read);
    auto shared = project.readObject("shared");
    auto fontScale = shared.read<float>("UIScale");
    stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART] = 
        shared.read<bool>("resetTimeOnRenderRestart");
    setMouseInputsEnabled(shared.read<bool>("iMouseInputEnabled"));
    setShaderCameraDirectionInputsEnabled
    (
        shared.read<bool>("iLookInputEnabled")
    );
    shaderCamera_->setMouseSensitivity
    (
        shared.read<float>("iLookSensitivity")
    );
    auto dir = shared.read<glm::vec3>("iLook");
    shaderCamera_->setDirection(dir);
    setShaderCameraPositionInputsEnabled
    (
        shared.read<bool>("iWASDInputEnabled")
    );
    shaderCamera_->setKeySensitivity
    (
        shared.read<float>("iWASDSensitivity")
    );
    auto pos = shared.read<glm::vec3>("iWASD");
    shaderCamera_->setPosition(pos);
    stateFlags_[ST_IS_TIME_PAUSED] = shared.read<bool>("timePaused");
    if (shared.hasMember("timeLooped"))
        stateFlags_[ST_IS_TIME_LOOPED] = shared.read<bool>("timeLooped");
    time_ = shared.read<float>("time");
    
    resetSharedUniforms();

    resourceManager_->loadState(project);
    layerManager_->loadState(project);
    //quantizationTool_->loadState(project);
    exportTool_->loadState(project);
    restartRendering(false);

    auto resolution = shared.read<glm::ivec2>("windowResolution");
    vir::GlobalPtr<vir::Window>::instance()->setSize
    (
        resolution.x, 
        resolution.y
    );
}

//----------------------------------------------------------------------------//

void ShaderThingApp::update()
{
    newProject();
    saveProject();
    loadProject();

    auto someLayersToBeCompiled = layerManager_->update();
    resourceManager_->update();
    exportTool_->update();

    // Update the rest
    auto window = vir::GlobalPtr<vir::Window>::instance();
    if (!stateFlags_[ST_IS_WINDOW_ICONIFIED])
    {   // TODO: make the base Camera class an event receiver and prevent it
        // from updating when the window is iconified, instead of having to 
        // do this other bits of code on top
        screenCamera_->update();
        shaderCamera_->update();
    }
    
    if (exportTool_->isExporting())
    {
        time_ += exportTool_->exportTimeStep();
        if (exportTool_->frame() <= 1)
            frame_ = 0; // Reset frame on export start
        else 
            frame_++;
    }
    else
    {
        // Recall that time is just a uniform with a one-way coupling to
        // rendering: if the rendering is going as usual, time is governed
        // by its own pause control, it the rendering is paused, pause the
        // time uniform as well for good measure
        static bool& isRenderingPaused(stateFlags_[ST_IS_RENDERING_PAUSED]);
        static bool& isTimePaused(stateFlags_[ST_IS_TIME_PAUSED]);
        static bool& isTimeLooped(stateFlags_[ST_IS_TIME_LOOPED]);
        if (!isRenderingPaused)
        {
            frame_++;
            if (!isTimePaused)
                time_ += window->time()->outerTimestep();
            if (isTimeLooped && time_ >= timeLoopBounds_->y)
            {
                auto w = timeLoopBounds_->y-timeLoopBounds_->x;
                auto f = (time_-timeLoopBounds_->y)/std::max(w, 1e-6f);
                f = f-(int)f;
                time_ = timeLoopBounds_->x + w*f;
            }
        }
        else if (!isTimePaused)
            isTimePaused = true;
    }

    // Update window title
    std::string projectTitle
    (
        projectFilename_ == "" ? 
        "untitled project" : 
        projectFilename_
    );
    if (someLayersToBeCompiled) // With 'compiling' info
        window->setTitle("ShaderThing (compiling "+projectTitle+")");
    else
    {
        static float fps(60.0f);
        static int elapsedFrames(0);
        static float elapsedTime(0);
        elapsedFrames++;
        elapsedTime += window->time()->outerTimestep();
        if (elapsedFrames >= int(fps/2.0f)) // Update every ~1/2 second
        {
            fps = elapsedFrames/elapsedTime;
            std::stringstream sfps;
            sfps << std::fixed << std::setprecision(1) << fps;
            window->setTitle
            (
                "ShaderThing ("+sfps.str()+" FPS, "+projectTitle+")"
            );
            elapsedFrames = 0;
            elapsedTime = 0;
        }
    }
    window->update(!stateFlags_[ST_IS_RENDERING_PAUSED]);
}

//----------------------------------------------------------------------------//

float ShaderThingApp::timeStep() const
{
    if (exportTool_->isExporting())
        return exportTool_->exportTimeStep();
    return vir::GlobalPtr<vir::Time>::instance()->outerTimestep();
}

}
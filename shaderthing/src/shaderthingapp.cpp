#include "shaderthingapp.h"
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

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

// Constructor/destructor ----------------------------------------------------//

ShaderThingApp::ShaderThingApp()
{
    // Initialize vir lib
    vir::initialize
    (
        vir::PlatformType::GLFWOpenGL, 
        resolution_.x, 
        resolution_.y, 
        "ShaderThing", 
        true
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
    quantizationTool_ = new QuantizationTool(*this);
    findReplaceTextTool_ = new FindReplaceTextTool();
    codeRepository_ = new CodeRepository();
    about_ = new About();
    restart();
    
    // Main loop
    while(window->isOpen())
    {
        // Rendering
        if (!isRenderingPausedCRef())
        {
            vir::Framebuffer* target = nullptr; // nullptr == to window
            uint32_t nRenderPasses = exportTool_->nRendersPerFrame();
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
    delete quantizationTool_;
    delete codeRepository_;
    delete about_;
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

void ShaderThingApp::restartRendering()
{
    frame_ = -1;
    if (stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART])
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
        vir::GlobalPtr<vir::InputState>::instance()->pressedMouseButtons()
        [
            VIR_MOUSE_BUTTON_1
        ]
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
// Private functions ---------------------------------------------------------//

void ShaderThingApp::restart()
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
    for (int i=0; i<13; i++)
        stateFlags_[i] = (i>6 && i<10);
    stateFlags_[ST_IS_TIME_RESET_ON_RENDER_RESTART] = true;
    // Restart app components
    layerManager_->reset();
    resourceManager_->reset();
    exportTool_->reset();
    quantizationTool_->reset();
    exportTool_->reset();

    static bool startup(true);
    if (startup)
    {
        // Set renderer options
        auto renderer = vir::GlobalPtr<vir::Renderer>::instance();
        renderer->setBlending(true);
        renderer->setFaceCulling(false);
        
        // Start ImGui
        vir::ImGuiRenderer::initialize();
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
    restart();
}

//----------------------------------------------------------------------------//

void ShaderThingApp::saveProject(){
    if (!stateFlags_[ST_SAVE_PROJECT]) return;
    stateFlags_[ST_SAVE_PROJECT] = false;

    std::ofstream file;
    file.open(projectFilepath_, std::ios_base::out | std::ios_base::binary);
    if(!file.is_open())
        return;

    // App data
    char data[150];
    const glm::vec3& cp(shaderCamera_->position());
    const glm::vec3& cz(shaderCamera_->z());
    std::sprintf
    (
        data,
        "%d %d %.9e %d %d %d %d %.9e %.9e %.9e %.9e %.9e %.9e", 
        resolution_.x, resolution_.y,
        time_, (int)stateFlags_[ST_IS_TIME_PAUSED], 
        (int)stateFlags_[ST_IS_CAMERA_POSITION_INPUT_ENABLED],
        (int)stateFlags_[ST_IS_CAMERA_DIRECTION_INPUT_ENABLED],
        (int)stateFlags_[ST_IS_MOUSE_INPUT_ENABLED],
        cp.x, cp.y, cp.z,
        cz.x, cz.y, cz.z
    );
    file << data << std::endl;

    // Component data
    resourceManager_->saveState(file);
    layerManager_->saveState(file);
    exportTool_->saveState(file);
    quantizationTool_->saveState(file);

    file.close();
}

//----------------------------------------------------------------------------//

void ShaderThingApp::loadProject()
{
    if (!stateFlags_[ST_LOAD_PROJECT]) return;
    stateFlags_[ST_LOAD_PROJECT] = false;

    std::ifstream file(projectFilepath_, std::ios_base::in | std::ios::binary);
    if(!file)
        return;
    std::string fileData;
    file.seekg(0, std::ios::end);
    fileData.resize(file.tellg());
    file.seekg(0, std::ios::beg);
    uint32_t fileDataSize(fileData.size());
    file.read(&fileData[0], fileDataSize);
    
    // Load app main state
    uint32_t index(0);
    std::string appSource;
    uint32_t width, height, timePaused, isCameraPositionInputEnabled, 
        isCameraDirectionInputEnabled, isMouseInputEnabled;
    glm::vec3 cp, cd;
    while(true)
    {
        char& c = fileData[index];
        if (c == '\n')
            break;
        appSource += c;
        index++;
    }
    index++;
    sscanf
    (
        appSource.c_str(),
        "%d %d %f %d %d %d %d %f %f %f %f %f %f",
        &width, &height,
        &time_, &timePaused, 
        &isCameraPositionInputEnabled,
        &isCameraDirectionInputEnabled,
        &isMouseInputEnabled,
        &cp.x, &cp.y, &cp.z,
        &cd.x, &cd.y, &cd.z
    );
    stateFlags_[ST_IS_TIME_PAUSED] = (bool)timePaused;
    setShaderCameraPositionInputsEnabled(isCameraPositionInputEnabled);
    setShaderCameraDirectionInputsEnabled(isCameraDirectionInputEnabled);
    setMouseInputsEnabled(isMouseInputEnabled);
    frame_ = 0; // Frame never saved as it might be used by shaders for init.
    stateFlags_[ST_IS_RENDERING_PAUSED] = false;
    shaderCamera_->setPosition(cp);
    shaderCamera_->setDirection(cd);
    auto window = vir::GlobalPtr<vir::Window>::instance();
    window->setSize(width, height);

    resourceManager_->loadState(fileData, index);
    layerManager_->loadState(fileData, index);
    quantizationTool_->loadState
    (
        fileData,
        index
    );
    exportTool_->loadState
    (
        fileData,
        index
    );
    file.close();

    restartRendering();
}

//----------------------------------------------------------------------------//

void ShaderThingApp::update()
{
    newProject();
    saveProject();
    loadProject();

    auto someLayersToBeCompiled = layerManager_->update();

    exportTool_->update();
    quantizationTool_->update();

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
        if (!isRenderingPaused)
        {
            frame_++;
            if (!isTimePaused)
                time_ += window->time()->outerTimestep();
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

    //
    window->update(!stateFlags_[ST_IS_RENDERING_PAUSED]);
}

}
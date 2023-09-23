#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "resources/resourcemanager.h"
#include "tools/quantizationtool.h"
#include "tools/exporttool.h"
#include "misc/misc.h"

#include "vir/include/vir.h"

#include <random>

namespace ShaderThing
{

//----------------------------------------------------------------------------//
//- Static members -----------------------------------------------------------//

vir::Shader* Layer::voidShader_ = nullptr;
vir::Shader* Layer::internalFramebufferShader_ = nullptr;
Layer::RendersTo Layer::rendererTargets[3] = 
{
    Layer::RendersTo::Window,
    Layer::RendersTo::InternalFramebuffer,
    Layer::RendersTo::InternalFramebufferAndWindow,
};
std::string Layer::rendererTargetNames[3] =
{
    "Window",
    "Framebuffer",
    "Framebuffer & window"
};
std::unordered_map<Layer::RendersTo, std::string> 
Layer::rendererTargetToName = 
{
    {rendererTargets[0], rendererTargetNames[0]},
    {rendererTargets[1], rendererTargetNames[1]},
    {rendererTargets[2], rendererTargetNames[2]}
};
std::unordered_map<std::string, Layer::RendersTo> 
Layer::nameToRendererTarget =
{
    {rendererTargetNames[0], rendererTargets[0]},
    {rendererTargetNames[1], rendererTargets[1]},
    {rendererTargetNames[2], rendererTargets[2]}
};
std::string Layer::supportedUniformTypeNames[11] = {
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Bool],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int2],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int3],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Int4],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float2],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float3],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Float4],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::Sampler2D],
    vir::Shader::uniformTypeToName[vir::Shader::Variable::Type::SamplerCube]
};

vir::TextureBuffer::InternalFormat Layer::supportedInternalFormats[2] = 
{
    vir::TextureBuffer::InternalFormat::RGBA_UNI_8, 
    vir::TextureBuffer::InternalFormat::RGBA_SF_32
};

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

// Constructor/Destructor ----------------------------------------------------//

Layer::Layer
(
    ShaderThingApp& app,
    glm::ivec2 resolution,
    float depth
) :
app_(app),
rendersTo_(RendersTo::Window),
toBeDeleted_(false),
toBeRenamed_(false),
toBeCompiled_(false),
isGuiRendered_(false),
isGuiDeletionConfirmationPending_(false),
isVertexEditorVisible_(false),
uncompiledVertexEditorChanges_(false),
uncompiledFragmentEditorChanges_(false),
depth_(depth),
resolution_(resolution),
targetResolution_(resolution),
resolutionScale_(1.0),
viewport_
(
    {
        (resolution.x > resolution.y) ? 1.0 : 
            float(resolution.x)/float(resolution.y),
        (resolution.y > resolution.x) ? 1.0 : 
            float(resolution.y)/float(resolution.x)
    }
),
vertexSource_
(
R"(#version 460 core

layout (location=0) in vec3 ipos;
layout (location=1) in vec2 itxc;
out vec2 pos;
out vec2 txc;

uniform mat4 mvp;

void main()
{
    gl_Position = mvp*vec4(ipos, 1.);
    pos = ipos.xy;
    txc = itxc;
})"
),
fragmentSource_(
R"(#version 460 core

out vec4 fragColor;
in vec2 pos;
in vec2 txc;
#define uv pos

uniform uint iFrame;
uniform float iAspectRatio;
uniform float iTime;
uniform vec2 iResolution;
uniform ivec4 iMouse;
uniform vec3 iCameraPosition;
uniform vec3 iCameraDirection;
uniform sampler2D iResource0;

void main()
{
    fragColor = vec4
    (
        .125*sin(uv.x*cos(uv.y-cos(2.5*iTime))*3.-1.5*iTime)+.125,
        .125*cos(uv.y*sin(uv.x-cos(2.0*iTime))*2.-2*iTime)+.500,
        .125*cos(uv.x*cos(uv.y-sin(1.5*iTime))*1.-2.5*iTime)+.700,
        1.0
    );
})"
),
screenQuad_(new vir::Quad(viewport_.x, viewport_.y, depth)),
time_(app.timeRef()),
timePaused_(app.isTimePausedRef()),
frame_(app.frameRef()),
screenCamera_(app.screnCameraRef()),
shaderCamera_(app.shaderCameraRef()),
renderer_(*vir::GlobalPtr<vir::Renderer>::instance()),
shaderId0_(-1),
uniformLayerNamesToBeSet_(0)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution
    (
        1000,
        1000000000
    );
    id_ = distribution(rng)+(int(time_)%1000);
    vertexEditor_.SetText(vertexSource_);
    vertexEditor_.SetLanguageDefinition
    (
        ImGuiExtd::TextEditor::LanguageDefinition::GLSL()
    );
    vertexEditor_.ResetTextChanged();
    fragmentEditor_.SetText(fragmentSource_);
    fragmentEditor_.SetLanguageDefinition
    (
        ImGuiExtd::TextEditor::LanguageDefinition::GLSL()
    );
    fragmentEditor_.ResetTextChanged();

    // Init shader
    shader_ =
        vir::Shader::create
        (
            vertexSource_, 
            fragmentSource_,
            vir::Shader::ConstructFrom::String
        );

    //
    createStaticShaders();

    // Init framebuffers
    flipFramebuffers_ = false;
    framebufferA_ = 
        vir::Framebuffer::create
        (
            resolution_.x, 
            resolution_.y
        );
    readOnlyFramebuffer_ = framebufferA_;
    framebufferB_ = 
        vir::Framebuffer::create
        (
            resolution_.x, 
            resolution_.y
        );
    writeOnlyFramebuffer_ = framebufferB_;

    // Init default uniforms
    initializeDefaultUniforms();

    ++LayerManager::nLayersSinceNewProject_;
}

//----------------------------------------------------------------------------//

Layer::~Layer()
{
    // Unregister from resource manager
    app_.resourceManagerRef().removeLayerAsResource(this);

    // (Try to) Unregister from quantizer
    app_.quantizationToolRef().removeLayerAsTarget(this);

    // Delete all managed resources
    delete screenQuad_;
    screenQuad_ = nullptr;
    delete shader_;
    shader_ = nullptr;
    for (auto u : defaultUniforms_)
        delete u;
    for (auto u : uniforms_)
        delete u;
    if (framebufferA_ != nullptr)
        delete framebufferA_;
    framebufferA_ = nullptr;
    if (framebufferB_ != nullptr)
        delete framebufferB_;
    framebufferB_ = nullptr;
    if (app_.layerManagerRef().layersRef().size() <= 1)
    {
        if (Layer::voidShader_ != nullptr)
            delete Layer::voidShader_;
        Layer::voidShader_ = nullptr;
        if (Layer::internalFramebufferShader_ != nullptr)
            delete Layer::internalFramebufferShader_;
        Layer::internalFramebufferShader_ = nullptr;
    }
}

//----------------------------------------------------------------------------//

void Layer::render(vir::Framebuffer* targetFramebuffer, bool clearTarget)
{
    if (toBeCompiled_)
    {
        compileShader();
        toBeCompiled_ = false;
    }

    // Flip buffers
    flipFramebuffers_ = !flipFramebuffers_;
    writeOnlyFramebuffer_ = flipFramebuffers_?framebufferA_:framebufferB_;
    readOnlyFramebuffer_ = flipFramebuffers_?framebufferB_:framebufferA_;

    // Set global uniforms
    setDefaultAndSamplerUniforms();

    // Render
    vir::Framebuffer* renderTarget // Nullptr means render to the window
    (
        rendersTo_ != RendersTo::Window ? 
        writeOnlyFramebuffer_ : 
        targetFramebuffer
    );
    
    renderer_.submit(*screenQuad_, shader_, renderTarget, clearTarget);
    if 
    (
        renderTarget != nullptr && 
        rendersTo_ != RendersTo::InternalFramebufferAndWindow
    )
        renderer_.submit(*screenQuad_, Layer::voidShader_);
}

//----------------------------------------------------------------------------//

void Layer::renderInternalFramebuffer
(
    vir::Framebuffer* targetFramebuffer, 
    bool clearTarget
)
{
    if (rendersTo_ != RendersTo::InternalFramebufferAndWindow)
        return;

    Layer::internalFramebufferShader_->bind();
    writeOnlyFramebuffer_->bindColorBuffer(0);
    Layer::internalFramebufferShader_->setUniformInt("self", 0);

    renderer_.submit
    (
        *screenQuad_, 
        Layer::internalFramebufferShader_, 
        targetFramebuffer,
        clearTarget
    );
    if (targetFramebuffer != nullptr)
        renderer_.submit(*screenQuad_, Layer::voidShader_);
}

//----------------------------------------------------------------------------//

void Layer::update()
{
    if (toBeRenamed_)
    {
        Misc::enforceUniqueItemName(targetName_, this, app_.layersRef());
        name_ = targetName_;
        toBeRenamed_ = false;
    }

    if (fragmentEditor_.IsTextChanged())
        uncompiledFragmentEditorChanges_ = 
            fragmentSource_!=fragmentEditor_.GetText();
    if (isVertexEditorVisible_ && vertexEditor_.IsTextChanged())
        uncompiledVertexEditorChanges_ = 
            vertexSource_!=vertexEditor_.GetText();

    if (resolution_ == targetResolution_)
        return;
    resolution_ = targetResolution_;
    app_.exportToolRef().updateLayerResolutions();

    float aspectRatio = 
        vir::GlobalPtr<vir::Window>::instance()->aspectRatio();
    viewport_.x = std::min(1.0f, aspectRatio);
    viewport_.y = std::min(1.0f, 1.0f/aspectRatio);
    screenQuad_->update(viewport_.x, viewport_.y, depth_);

    // Resize framebuffers by rebuilding them with the new resolution and with
    // the same internal format as before (by construction both the front
    // and back-buffers need to have the same internal format for the color
    // attachment texture)
    rebuildFramebuffers
    (
        framebufferA_->colorBufferInternalFormat(), 
        resolution_
    );
}

//----------------------------------------------------------------------------//

void Layer::removeResourceFromUniforms(Resource* resource)
{
    int index = -1;
    for (auto u : uniforms_)
    {
        index++;
        if 
        (
            u->type != vir::Shader::Variable::Type::Sampler2D && 
            u->type != vir::Shader::Variable::Type::SamplerCube
        )
            continue;
        auto r = u->getValuePtr<Resource>();
        if (r == nullptr)
            continue;
        if (r->name() != resource->name())
            continue;
        uniforms_.erase(uniforms_.begin()+index);
        uniformLimits_.erase(u);
        index--;
    }
}

//----------------------------------------------------------------------------//

void Layer::setName(std::string name)
{
    name_ = name;
    targetName_ = name;
}

//----------------------------------------------------------------------------//

void Layer::setDepth(float depth)
{
    screenQuad_->update(viewport_.x, viewport_.y, depth);
}

//----------------------------------------------------------------------------//

void Layer::setTargetResolution(glm::ivec2 resolution, bool rescale)
{
    if (rescale)
        targetResolution_ = 
            (glm::vec2)resolution*resolutionScale_ + 
            glm::vec2({.5,.5});
    else
        targetResolution_ = resolution;
    targetResolution_.x = std::max(targetResolution_.x, 1);
    targetResolution_.y = std::max(targetResolution_.y, 1);
}

//----------------------------------------------------------------------------//

// Constructor from source file / deserialization
Layer::Layer
(
    ShaderThingApp& app,
    std::string& source,
    uint32_t& index,
    bool isGuiRendered
) :
app_(app),
rendersTo_(RendersTo::Window),
toBeDeleted_(false),
toBeRenamed_(false),
isGuiRendered_(isGuiRendered),
isGuiDeletionConfirmationPending_(false),
toBeCompiled_(false),
isVertexEditorVisible_(false),
uncompiledVertexEditorChanges_(false),
uncompiledFragmentEditorChanges_(false),
time_(app.timeRef()),
timePaused_(app.isTimePausedRef()),
frame_(app.frameRef()),
screenCamera_(app.screnCameraRef()),
shaderCamera_(app.shaderCameraRef()),
renderer_(*vir::GlobalPtr<vir::Renderer>::instance()),
shaderId0_(-1),
uniformLayerNamesToBeSet_(0)
{
    std::string headerSource;
    while(true)
    {
        char& c = source[index];
        if (c == '\n')
            break;
        headerSource += c;
        index++;
    }
    index++;

    auto layerName = new char[headerSource.size()];
    uint32_t rendersTo, isVertexEditorVisible, showSelfPreview, vertexSourceSize, 
        fragmentSourceSize, nUniforms, xWrap, yWrap, magFilter, minFilter;
    sscanf
    (
        headerSource.c_str(),
        "%s %d %d %f %f %d %d %d %d %d %d %d %d %d",
        layerName, 
        &resolution_.x, &resolution_.y, &depth_, &resolutionScale_, 
        &rendersTo, &isVertexEditorVisible,
        &xWrap, &yWrap, &magFilter, &minFilter,
        &vertexSourceSize, &fragmentSourceSize, &nUniforms
    );
    name_ = layerName;
    targetName_ = layerName;
    delete[] layerName;
    rendersTo_ = (RendersTo)rendersTo;
    isVertexEditorVisible_ = (bool)isVertexEditorVisible;
    vertexSource_.resize(vertexSourceSize);
    fragmentSource_.resize(fragmentSourceSize);

    // Parse vertex, fragment source -------------------------------------------
    uint32_t index0(index);
    while(index-index0 < vertexSourceSize)
    {
        vertexSource_[index-index0] = source[index];
        index++;
    }
    index0 = ++index;
    while(index-index0 < fragmentSourceSize)
    {
        fragmentSource_[index-index0] = source[index];
        index++;
    }
    index0 = ++index;

    // Parse uniforms ----------------------------------------------------------
    bool readingType(true);
    std::string uniformTypeName;
    std::string uniformSource;
    char uniformName[200];
    index0 = index;
    while(uniforms_.size() < nUniforms)
    {
        char& c = source[index];
        if(c != '\n')
        {
            if (readingType && c == ' ')
                readingType = false;
            else
                (readingType ? uniformTypeName : uniformSource) += c;
            index++;
            continue;
        }
        auto uniform = new vir::Shader::Uniform();
        uniform->type = vir::Shader::uniformNameToType[uniformTypeName];
        uniforms_.emplace_back(uniform);
        uniformUsesColorPicker_.insert({uniform, false});
        bool* uniformUsesColorPicker = &uniformUsesColorPicker_[uniform];
        float min, max, x, y, z, w;
        switch (uniform->type)
        {
            case vir::Shader::Variable::Type::Bool :
            {
                int value;
                sscanf
                (
                    uniformSource.c_str(),
                    "%d %f %f %s",
                    &value, &min, &max, 
                    &uniformName
                );
                uniform->setValue<bool>((bool)value);
                break;
            }
            case vir::Shader::Variable::Type::Int :
            {
                int value;
                sscanf
                (
                    uniformSource.c_str(),
                    "%d %f %f %s",
                    &value, &min, &max, 
                    &uniformName
                );
                uniform->setValue<int>(value);
                break;
            }
            case vir::Shader::Variable::Type::Float :
            {
                sscanf
                (
                    uniformSource.c_str(),
                    "%f %f %f %s",
                    &x, &min, &max,
                    &uniformName
                );
                uniform->setValue<float>(x);
                break;
            }
            case vir::Shader::Variable::Type::Float2 :
            {
                sscanf
                (
                    uniformSource.c_str(),
                    "%f %f %f %f %s",
                    &x, &y, &min, &max, 
                    &uniformName
                );
                uniform->setValue<glm::vec2>({x,y});
                break;
            }
            case vir::Shader::Variable::Type::Float3 :
            {
                int useColorPicker(false);
                sscanf
                (
                    uniformSource.c_str(),
                    "%f %f %f %f %f %d %s",
                    &x, &y, &z, &min, &max, &useColorPicker,
                    &uniformName
                );
                if (useColorPicker)
                    *uniformUsesColorPicker = true;
                uniform->setValue<glm::vec3>({x,y,z});
                break;
            }
            case vir::Shader::Variable::Type::Float4 :
            {
                int useColorPicker(false);
                sscanf
                (
                    uniformSource.c_str(),
                    "%f %f %f %f %f %f %d %s",
                    &x, &y, &z, &w, &min, &max, &useColorPicker,
                    &uniformName
                );
                if (useColorPicker)
                    *uniformUsesColorPicker = true;
                uniform->setValue<glm::vec4>({x,y,z,w});
                break;
            }
            case vir::Shader::Variable::Type::Sampler2D :
            case vir::Shader::Variable::Type::SamplerCube :
            {
                char resourceName[200];
                sscanf
                (
                    uniformSource.c_str(),
                    "%s %s",
                    &resourceName, &uniformName
                );
                min = 0;
                max = 0;
                bool found = false;
                for (auto r : app.resourceManagerRef().resourcesRef())
                {
                    if (r->name() == resourceName)
                    {
                        uniform->setValuePtr<Resource>(r);
                        found = true;
                        break;
                    }
                }
                if (!found)
                    uniformLayerNamesToBeSet_.insert
                    (
                        {uniform, resourceName}
                    );
                break;
            }
        }
        uniformLimits_.insert({uniform, {min, max}});
        uniform->name = uniformName;
        uniformTypeName = "";
        uniformSource = "";
        readingType = true;
        index++;
    }

    // Initialize all other variables ------------------------------------------
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution
    (
        1000,
        1000000000
    );
    id_ = distribution(rng)+(int(time_)%1000);
    targetResolution_ = resolution_;
    viewport_.x = (resolution_.x > resolution_.y) ? 1.0 : 
        float(resolution_.x)/float(resolution_.y);
    viewport_.y = (resolution_.y > resolution_.x) ? 1.0 : 
        float(resolution_.y)/float(resolution_.x);
    screenQuad_ = new vir::Quad(viewport_.x, viewport_.y, depth_);
    vertexEditor_.SetText(vertexSource_);
    vertexEditor_.SetLanguageDefinition
    (
        ImGuiExtd::TextEditor::LanguageDefinition::GLSL()
    );
    vertexEditor_.ResetTextChanged();
    fragmentEditor_.SetText(fragmentSource_);
    fragmentEditor_.SetLanguageDefinition
    (
        ImGuiExtd::TextEditor::LanguageDefinition::GLSL()
    );
    fragmentEditor_.ResetTextChanged();

    // Init shader
    shader_ =
        vir::Shader::create
        (
            vertexSource_, 
            fragmentSource_,
            vir::Shader::ConstructFrom::String
        );

    //
    createStaticShaders();

    // Init framebuffers
    flipFramebuffers_ = false;
    framebufferA_ = vir::Framebuffer::create(resolution_.x, resolution_.y);
    readOnlyFramebuffer_ = framebufferA_;
    framebufferB_ = vir::Framebuffer::create(resolution_.x, resolution_.y);
    writeOnlyFramebuffer_ = framebufferB_;

    // Set framebuffer color attachment wrapping and filtering options
    auto setWrapFilterOptions = [&](vir::Framebuffer* framebuffer)->void
    {
        framebuffer->setColorBufferMagFilterMode
        (
            (vir::TextureBuffer::FilterMode)magFilter
        );
        framebuffer->setColorBufferMinFilterMode
        (
            (vir::TextureBuffer::FilterMode)minFilter
        );
        framebuffer->setColorBufferWrapMode
        (
            0,
            (vir::TextureBuffer::WrapMode)xWrap
        );
        framebuffer->setColorBufferWrapMode
        (
            1,
            (vir::TextureBuffer::WrapMode)yWrap
        );
    };
    setWrapFilterOptions(framebufferA_);
    setWrapFilterOptions(framebufferB_);

    //
    if (rendersTo_ != RendersTo::Window)
        app.resourceManagerRef().addLayerAsResource(this);

    // Init default uniforms
    initializeDefaultUniforms();

    ++LayerManager::nLayersSinceNewProject_;
}

// De-/serialization functions -----------------------------------------------//

void Layer::saveState(std::ofstream& file)
{
    char data[200];
    
    // Self-rendered texture wrapping and filtering options
    int xWrap = 0, yWrap = 0, magFilter = 0, minFilter = 0;
    if (readOnlyFramebuffer_ != nullptr)
    {
        xWrap = int(readOnlyFramebuffer_->colorBufferWrapMode(0));
        yWrap = int(readOnlyFramebuffer_->colorBufferWrapMode(1));
        magFilter = int(readOnlyFramebuffer_->colorBufferMagFilterMode());
        minFilter = int(readOnlyFramebuffer_->colorBufferMagFilterMode());
    }
    sprintf
    (
        data, 
        "%s %d %d %.9f %.9f %d %d %d %d %d %d %d %d %d", 
        name_.c_str(), 
        resolution_.x, resolution_.y, 
        depth_, resolutionScale_, 
        (int)rendersTo_, (int)isVertexEditorVisible_, 
        xWrap, yWrap, magFilter, minFilter,
        vertexSource_.size(), fragmentSource_.size(), (int)uniforms_.size()
    );
    file << data << std::endl;

    // Vertex, fragment data
    file << vertexSource_ << std::endl;
    file << fragmentSource_ << std::endl;
    
    // Uniform data
    for (auto u : uniforms_)
    {
        glm::vec2& uLimits(uniformLimits_[u]);
        float& min(uLimits.x);
        float& max(uLimits.y);
        file << vir::Shader::uniformTypeToName[u->type] << " ";
        switch(u->type)
        {
            case vir::Shader::Variable::Type::Bool :
                sprintf(data, "%d", u->getValue<bool>());
                break;
            case vir::Shader::Variable::Type::Int :
                sprintf(data, "%d %.0f %.0f", u->getValue<int>(), min, max);
                break;
            case vir::Shader::Variable::Type::Float :
                sprintf(data, "%.9e %.9e %.9e", u->getValue<float>(), min, max);
                break;
            case vir::Shader::Variable::Type::Float2 :
            {
                auto v = u->getValue<glm::vec2>();
                sprintf(data, "%.9e %.9e %.9e %.9e", v.x, v.y, min, max);
                break;
            }
            case vir::Shader::Variable::Type::Float3 :
            {
                auto v = u->getValue<glm::vec3>();
                bool usesColorPicker = false;
                if 
                (
                    uniformUsesColorPicker_.find(u) != 
                    uniformUsesColorPicker_.end()
                )
                    usesColorPicker = uniformUsesColorPicker_[u];
                sprintf
                (
                    data, 
                    "%.9e %.9e %.9e %.9e %.9e %d", 
                    v.x, v.y, v.z, min, max, int(usesColorPicker)
                );
                break;
            }
            case vir::Shader::Variable::Type::Float4 :
            {
                auto v = u->getValue<glm::vec4>();
                bool usesColorPicker = false;
                if 
                (
                    uniformUsesColorPicker_.find(u) != 
                    uniformUsesColorPicker_.end()
                )
                    usesColorPicker = uniformUsesColorPicker_[u];
                sprintf
                (
                    data, 
                    "%.9e %.9e %.9e %.9e %.9e %.9e %d", 
                    v.x, v.y, v.z, v.w, min, max, usesColorPicker
                );
                break;
            }
            case vir::Shader::Variable::Type::Sampler2D :
            case vir::Shader::Variable::Type::SamplerCube :
            {
                auto r = u->getValuePtr<Resource>();
                sprintf(data, "%s", r->name().c_str());
                break;
            }
            default:
                break;
        }
        file << data << " " << u->name << std::endl;
    }
}

//----------------------------------------------------------------------------//

void Layer::reBindLayerUniforms()
{
    for (auto& entry : uniformLayerNamesToBeSet_)
    {
        auto* uniform = entry.first;
        auto& layerName = entry.second;
        for (auto r : app_.resourceManagerRef().resourcesRef())
        {
            if (r->name() == layerName)
            {
                uniform->setValuePtr<Resource>(r);
                break;
            }
        }
    }
    uniformLayerNamesToBeSet_.clear();
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

void Layer::compileShader()
{
    if (!uncompiledVertexEditorChanges_ && !uncompiledFragmentEditorChanges_)
        return;

    vertexSource_ = vertexEditor_.GetText();
    fragmentSource_ = fragmentEditor_.GetText();

    vir::Shader* tmp = nullptr;
    std::exception_ptr exceptionPtr;
    tmp = vir::Shader::create
    (
        vertexSource_, 
        fragmentSource_, 
        vir::Shader::ConstructFrom::String,
        &exceptionPtr
    );
    if (tmp != nullptr)
    {
        frame_ = 0;
        delete shader_;
        shader_ = tmp;
        tmp = nullptr;
        vertexEditor_.SetErrorMarkers({});
        fragmentEditor_.SetErrorMarkers({});
        uncompiledVertexEditorChanges_ = false;
        uncompiledFragmentEditorChanges_ = false;
        return;
    }
    try {std::rethrow_exception(exceptionPtr);}
    catch(std::exception& e)
    {
        // If the compilation fails, parse the error and show it
        // on the editor
        uncompiledVertexEditorChanges_ = true;
        uncompiledFragmentEditorChanges_ = true;
        std::string exception(e.what());
        std::cout << exception << std::endl;
        bool isVertex = true;
        if (exception[0] == '[' && exception[1] == 'F' &&
            exception[2] == ']')
            isVertex = false;
        bool readErrorIndex = true;
        int firstErrorIndex = -1;
        int errorIndex = 0;
        std::string errorIndexString = "";
        std::string error = "";
        ImGuiExtd::TextEditor::ErrorMarkers errors;
        uint32_t i = 3;
        while(i < exception.size())
        {
            char ei(exception[i]);
            char eip1(exception[std::min((uint32_t)exception.size()-1, i+1)]);
            if (readErrorIndex && (ei == '0' && (eip1 == '(' || eip1 == ':')))
            {
                i += 2;
                while(exception[i] != ')' && exception[i] != ':')
                    errorIndexString += exception[i++];
                errorIndex = std::stoi(errorIndexString);
                errorIndexString = "";
                if (firstErrorIndex == -1)
                    firstErrorIndex = errorIndex;
                readErrorIndex = false;
                i++;
            }
            else
            {
                if (ei != '\n')
                    error += ei;
                else
                {
                    errors.insert({errorIndex, error});
                    error = "";
                    readErrorIndex = true;
                }
            }
            i++;
        }
        if (isVertex)
        {
            vertexEditor_.SetErrorMarkers(errors);
            auto pos = ImGuiExtd::TextEditor::Coordinates(firstErrorIndex,0);
            vertexEditor_.SetCursorPosition(pos);
        }
        else 
        {
            fragmentEditor_.SetErrorMarkers(errors);
            auto pos = ImGuiExtd::TextEditor::Coordinates(firstErrorIndex,0);
            fragmentEditor_.SetCursorPosition(pos);
        }
    }
}

//----------------------------------------------------------------------------//

void Layer::createStaticShaders()
{
    if (Layer::voidShader_ == nullptr)
    {
        std::string fragmentSource = 
R"(#version 460 core
out vec4 fragColor;
in vec2 pos;
in vec2 txc;
void main(){fragColor = vec4(0.0,0,0,0.0);})";
        Layer::voidShader_ = vir::Shader::create
        (
            vertexSource_, 
            fragmentSource,
            vir::Shader::ConstructFrom::String
        );
    }
    if (Layer::internalFramebufferShader_ == nullptr)
    {
        std::string fragmentSource = 
R"(#version 460 core
out vec4 fragColor;
in vec2 pos;
in vec2 txc;
uniform sampler2D self;
void main(){fragColor = texture(self,txc);})";
        Layer::internalFramebufferShader_ = vir::Shader::create
        (
            vertexSource_, 
            fragmentSource,
            vir::Shader::ConstructFrom::String
        );
    }
}

//----------------------------------------------------------------------------//

void Layer::initializeDefaultUniforms()
{
    // Set default uniforms (mvp not here because not useful to visualize)

    // iFrame
    auto frameUniform = new vir::Shader::Uniform();
    frameUniform->name = "iFrame";
    frameUniform->type = vir::Shader::Variable::Type::UInt;
    frameUniform->setValuePtr(&frame_);
    defaultUniforms_.emplace_back(frameUniform);
    uniformLimits_.insert({frameUniform, glm::vec2(0.0f, 1.0f)});

    // iAspectRatio
    auto aspectRatio = new vir::Shader::Uniform();
    aspectRatio->name = "iAspectRatio";
    aspectRatio->type = vir::Shader::Variable::Type::Float;
    aspectRatio->setValuePtr
    (
        &vir::GlobalPtr<vir::Window>::instance()->aspectRatio()
    );
    defaultUniforms_.emplace_back(aspectRatio);
    uniformLimits_.insert({aspectRatio, glm::vec2(0.0f, 1.0f)});

    // iResolution
    auto resolutionUniform = new vir::Shader::Uniform();
    resolutionUniform->name = "iResolution";
    resolutionUniform->type = vir::Shader::Variable::Type::Float2;
    resolutionUniform->setValuePtr(&targetResolution_);
    defaultUniforms_.emplace_back(resolutionUniform);
    uniformLimits_.insert({resolutionUniform, glm::vec2(1.0f, 4096.0f)});
    
    // iTime
    auto timeUniform = new vir::Shader::Uniform();
    timeUniform->name = "iTime";
    timeUniform->type = vir::Shader::Variable::Type::Float;
    timeUniform->setValuePtr(&time_);
    defaultUniforms_.emplace_back(timeUniform);
    uniformLimits_.insert({timeUniform, glm::vec2(0.0f, 1.0f)});

    // iMouse
    auto mouseUniform = new vir::Shader::Uniform();
    mouseUniform->name = "iMouse";
    mouseUniform->type = vir::Shader::Variable::Type::Int4;
    mouseUniform->setValuePtr(&(app_.mouseRef()));
    defaultUniforms_.emplace_back(mouseUniform);
    uniformLimits_.insert({mouseUniform, glm::vec2(-1.f, 4096.f)});

    // Shader camera position
    auto cameraPositionUniform = new vir::Shader::Uniform();
    cameraPositionUniform->name = "iCameraPosition";
    cameraPositionUniform->type = vir::Shader::Variable::Type::Float3;
    cameraPositionUniform->setValuePtr(&shaderCamera_.position());
    defaultUniforms_.emplace_back(cameraPositionUniform);
    uniformLimits_.insert({cameraPositionUniform, glm::vec2(-1.0f, 1.0f)});

    // Shader camera direction
    auto cameraDirectionUniform = new vir::Shader::Uniform();
    cameraDirectionUniform->name = "iCameraDirection";
    cameraDirectionUniform->type = vir::Shader::Variable::Type::Float3;
    cameraDirectionUniform->setValuePtr(&shaderCamera_.z());
    defaultUniforms_.emplace_back(cameraDirectionUniform);
    uniformLimits_.insert({cameraDirectionUniform, glm::vec2(-1.0f, 1.0f)});
}

//----------------------------------------------------------------------------//

void Layer::setDefaultAndSamplerUniforms()
{
    // Check for shader recompilation (then, uniforms need to be
    // reset)
    static const glm::mat4& mvp(screenCamera_.projectionViewMatrix());
    static const glm::vec3& cameraPosition(shaderCamera_.position());
    static const glm::vec3& cameraDirection(shaderCamera_.z());
    static const glm::ivec4& mouse(app_.mouseRef());
    static const float& aspectRatio
    (
        vir::GlobalPtr<vir::Window>::instance()->aspectRatio()
    );
    shader_->bind();

    // Set default uniforms
    bool forceSet((int)shader_->id() != shaderId0_);
    shader_->setUniformFloat("iTime", time_);
    shader_->setUniformUInt("iFrame", frame_);
    if (mvp0_ != mvp || forceSet)
    {
        shader_->setUniformMat4("mvp", mvp);
        Layer::voidShader_->bind();
        Layer::voidShader_->setUniformMat4("mvp", mvp);
        Layer::internalFramebufferShader_->bind();
        Layer::internalFramebufferShader_->setUniformMat4("mvp", mvp);
        shader_->bind();
        mvp0_ = mvp;
    }
    if (resolution0_ != resolution_ || forceSet)
    {
        shader_->setUniformFloat("iAspectRatio", aspectRatio);
        shader_->setUniformFloat2("iResolution", resolution_);
        resolution0_ = resolution_;
    }
    if (cameraPosition0_ != shaderCamera_.position()  || forceSet)
    {
        shader_->setUniformFloat3("iCameraPosition", cameraPosition);
        cameraPosition0_ = cameraPosition;
    }
    if (cameraDirection0_ != cameraDirection || forceSet)
    {
        shader_->setUniformFloat3("iCameraDirection", cameraDirection);
        cameraDirection0_ = cameraDirection;
    }
    if (mouse0_ != mouse || forceSet)
    {
        shader_->setUniformInt4("iMouse", mouse);
        mouse0_ = mouse;
    }

    // Set sampler uniforms
    uint32_t unit = 0; 
    for (auto u : uniforms_)
    {
        if 
        (
            u->name == "" || 
            (
                u->type != vir::Shader::Variable::Type::Sampler2D &&
                u->type != vir::Shader::Variable::Type::SamplerCube
            )
        )
            continue;
        auto resource = u->getValuePtr<Resource>();
        if (resource == nullptr)
            continue;
        // These two lines are required when returning writeOnlyFramebuffer_
        // in readOnlyFramebuffer() to avoid visual artifacts. Depending on
        // the outcome of my experiments, I might keep this approach
        if (resource->namePtr() == &name_)
            readOnlyFramebuffer_->bindColorBuffer(unit);
        else
            resource->bind(unit);
        shader_->setUniformInt(u->name, unit);
        unit++;
    }

    // Set all other uniforms (reset if shader changed, otherwise they are 
    // already set in the gui rendering function)
    if (forceSet)
        setNonDefaultUniforms();

    shaderId0_ = shader_->id();
}

//----------------------------------------------------------------------------//

void Layer::setNonDefaultUniforms()
{

#define CASE(ST, T, F)                          \
case vir::Shader::Variable::Type::ST :          \
{                                               \
    auto value = u->getValue<T>();              \
    u->setValue(value);                         \
    if (u->name != "")                          \
        shader_->F(u->name, value);             \
    break;                                      \
}
    for (auto u : uniforms_)
    {
        switch(u->type)
        {
            CASE(Bool, bool, setUniformBool)
            CASE(Int, int, setUniformInt)
            CASE(Int2, glm::ivec2, setUniformInt2)
            CASE(Float, float, setUniformFloat)
            CASE(Float2, glm::vec2, setUniformFloat2)
            CASE(Float3, glm::vec3, setUniformFloat3)
            CASE(Float4, glm::vec4, setUniformFloat4)
            default:
                continue;
        }
    }
}

//----------------------------------------------------------------------------//

void Layer::adjustTargetResolution()
{
    /*
    The goal of this section of code is to:
    -   have window-aspect-ratio-bound layer width and
        height when editing if the layer is internally
        rendered (i.e., to self)
    -   have independently-modifiable window width and
        height if the layer is rendered to screen,
        and have the edit propagate throughout all 
        layers, so that self-rendered layers have 
        their aspect ratios (but not absolute dims)
        affected too
    */
    glm::ivec2 value0 = resolution_;
    glm::ivec2& value(targetResolution_);
    auto window = 
        vir::GlobalPtr<vir::Window>::instance();
    static const float& aspectRatio
    (
        window->aspectRatio()
    );
    static glm::ivec2 minResolution;
    static glm::ivec2 maxResolution;
    if (rendersTo_ != RendersTo::Window)
    {
        minResolution = {1,1};
        maxResolution = {4096,2160};
        if (value0.x != value.x)
            value.y = value.x/aspectRatio;
        else if (value0.y != value.y)
            value.x = value.y*aspectRatio;
    }
    else
    {
        auto monitorResolution = 
            window->primaryMonitorResolution();
        auto monitorScale = window->contentScale();
        minResolution = {120*monitorScale.x,1};
        maxResolution = monitorResolution;
    }
    value.x = std::max(minResolution.x, value.x);
    value.x = std::min(maxResolution.x, value.x);
    value.y = std::max(minResolution.y, value.y);
    value.y = std::min(maxResolution.y, value.y);
    if (rendersTo_ != RendersTo::Window)
    {
        if (value.x > value.y)
            resolutionScale_ = 
                (float)value.x/window->width();
        else
            resolutionScale_ = 
                (float)value.y/window->height();
    }
    else if (value0 != value)
    {
        window->setSize(value.x, value.y);
        vir::GlobalPtr
        <
            vir::Event::Broadcaster
        >::instance()->broadcast
        (
            vir::Event::WindowResizeEvent
            (
                value.x, 
                value.y
            )
        );
    }
}

//----------------------------------------------------------------------------//

void Layer::rebuildFramebuffers
(
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution
)
{
    auto rebuildFramebuffer = []
    (
        vir::Framebuffer*& framebuffer, 
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    )
    {
        auto wrapModeX = framebuffer->colorBufferWrapMode(0);
        auto wrapModeY = framebuffer->colorBufferWrapMode(1);
        auto minFilterMode = framebuffer->colorBufferMinFilterMode();
        auto magFilterMode = framebuffer->colorBufferMagFilterMode();
        framebuffer->unbind();
        if (framebuffer != nullptr)
            delete framebuffer;
        framebuffer = vir::Framebuffer::create
        (
            resolution.x, 
            resolution.y, 
            internalFormat
        );
        framebuffer->setColorBufferWrapMode(0, wrapModeX);
        framebuffer->setColorBufferWrapMode(1, wrapModeY);
        framebuffer->setColorBufferMinFilterMode(minFilterMode);
        framebuffer->setColorBufferMagFilterMode(magFilterMode);
    };

    rebuildFramebuffer(framebufferA_, internalFormat, resolution);
    rebuildFramebuffer(framebufferB_, internalFormat, resolution);
    writeOnlyFramebuffer_ = flipFramebuffers_?framebufferA_:framebufferB_;
    readOnlyFramebuffer_ = flipFramebuffers_?framebufferB_:framebufferA_;
}

}
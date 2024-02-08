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

#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "resources/resourcemanager.h"
#include "postprocess/postprocess.h"
#include "tools/quantizationtool.h"
#include "tools/exporttool.h"
#include "misc/misc.h"
#include "objectio/objectio.h"

#include "vir/include/vir.h"

#include "thirdparty/glad/include/glad/glad.h"

#include <random>

namespace ShaderThing
{

//----------------------------------------------------------------------------//
//- Static members -----------------------------------------------------------//

// Private -------------------------------------------------------------------//

vir::Quad* Layer::blankQuad_ = nullptr;
std::string Layer::blankFragmentSource_ = "";
vir::Shader* Layer::blankShader_ = nullptr;
vir::Shader* Layer::internalFramebufferShader_ = nullptr;
ImGuiExtd::TextEditor Layer::sharedSourceEditor_ = ImGuiExtd::TextEditor();
bool Layer::sharedSourceHasErrors_ = false;

std::unordered_map<Layer::RendersTo, std::string> 
Layer::renderTargetToName = 
{
    {Layer::RendersTo::InternalFramebufferAndWindow, "Framebuffer & window"},
    {Layer::RendersTo::InternalFramebuffer, "Framebuffer"},
    {Layer::RendersTo::Window, "Window"}
};

std::unordered_map
<
    Layer::InternalFramebufferClearPolicyOnExport, 
    std::string
> Layer::internalFramebufferClearPolicyOnExportToName = 
{
    {
        Layer::InternalFramebufferClearPolicyOnExport::ClearOnEveryFrame, 
        "On every frame"
    },
    {
        Layer::InternalFramebufferClearPolicyOnExport::ClearOnFirstFrame, 
        "On first frame"
    },
    {Layer::InternalFramebufferClearPolicyOnExport::None, "None"}
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

std::string Layer::defaultVertexSource_ = 
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
uniform mat4 mvp;
void main()
{
    gl_Position = mvp*vec4(iqc, 1.);
    qc = iqc.xy;
    tc = itc;
})";

std::string Layer::defaultFragmentSource_ = 
/*
R"(void main()
{
    fragColor = vec4
    (
        .125*sin(qc.x*cos(qc.y-cos(2.5*iTime))*3.-1.5*iTime)+.125,
        .125*cos(qc.y*sin(qc.x-cos(2.0*iTime))*2.-2*iTime)+.500,
        .125*cos(qc.x*cos(qc.y-sin(1.5*iTime))*1.-2.5*iTime)+.700,
        1.0
    );
})";*/
R"(void main()
{
    fragColor = vec4
    (
        .5+.25*sin(2*(qc+iTime)),
        .75,
        1.
    );
}
)";

std::string Layer::defaultSharedSource_ = 
R"(// Common source code is shared by all fragment shaders across all layers and
// has access to all shared in/out/uniform declarations

// Fragment coordinates
vec2 fragCoord = gl_FragCoord.xy; 

// Keyboard defs for convenience. To access the state of a key, use the ivec3
// iKeboard[KEY_XXX] uniform, where KEY_XXX is replaced by one of the defs here
// below. The three components .x, .y, .z are 1 if the key is pressed (but not
// held), held, toggled respectively, 0 otherwise
#define KEY_TAB 9
#define KEY_LEFT 37
#define KEY_RIGHT 39
#define KEY_UP 38
#define KEY_DOWN 40
#define KEY_DELETE 46
#define KEY_BACKSPACE 8
#define KEY_SPACE 32
#define KEY_ENTER 13
#define KEY_ESCAPE 27
#define KEY_APOSTROPHE 222
#define KEY_COMMA 188
#define KEY_MINUS 189
#define KEY_PERIOD 190
#define KEY_SLASH 191
#define KEY_SEMICOLON 186
#define KEY_EQUAL 187
#define KEY_LEFT_BRACKET 219
#define KEY_BACKSLASH 220
#define KEY_RIGHT_BRACKET 221
#define KEY_GRAVE_ACCENT 192
#define KEY_CAPS_LOCK 20
#define KEY_LEFT_SHIFT 16
#define KEY_LEFT_CONTROL 17
#define KEY_LEFT_ALT 18
#define KEY_LEFT_SUPER 91
#define KEY_RIGHT_SHIFT 16
#define KEY_RIGHT_CONTROL 17
#define KEY_RIGHT_ALT 18
#define KEY_0 48
#define KEY_1 49
#define KEY_2 50
#define KEY_3 51
#define KEY_4 52
#define KEY_5 53
#define KEY_6 54
#define KEY_7 55
#define KEY_8 56
#define KEY_9 57
#define KEY_A 65
#define KEY_B 66
#define KEY_C 67
#define KEY_D 68
#define KEY_E 69
#define KEY_F 70
#define KEY_G 71
#define KEY_H 72
#define KEY_I 73
#define KEY_J 74
#define KEY_K 75
#define KEY_L 76
#define KEY_M 77
#define KEY_N 78
#define KEY_O 79
#define KEY_P 80
#define KEY_Q 81
#define KEY_R 82
#define KEY_S 83
#define KEY_T 84
#define KEY_U 85
#define KEY_V 86
#define KEY_W 87
#define KEY_X 88
#define KEY_Y 89
#define KEY_Z 90
#define KEY_F1 112
#define KEY_F2 113
#define KEY_F3 114
#define KEY_F4 115
#define KEY_F5 116
#define KEY_F6 117
#define KEY_F7 118
#define KEY_F8 119
#define KEY_F9 120
#define KEY_F10 121
#define KEY_F11 122
#define KEY_F12 123

// For convenience when importing ShaderToy shaders
#define MainFromShaderToy void main(){mainImage(fragColor, fragCoord);}
)";

//----------------------------------------------------------------------------//

const std::string& Layer::assembleVertexSource()
{
    if (defaultVertexSource_[0] != '#')
    {
        static std::string version
        (
            "#version "+
            vir::GlobalPtr<vir::Window>::instance()->context()->
            shadingLanguageVersion()+
            " core\n"
        );
        defaultVertexSource_ = version+defaultVertexSource_;
    }
    return defaultVertexSource_;
}

//----------------------------------------------------------------------------//

void Layer::createStaticShaders()
{
    static std::string version(
    "#version "+
    vir::GlobalPtr<vir::Window>::instance()->context()->
    shadingLanguageVersion()+
    " core\n");
    if (Layer::blankShader_ == nullptr)
    {
        Layer::blankFragmentSource_ = 
version+
R"(out vec4 fragColor;
in vec2 qc;
in vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})";
        Layer::blankShader_ = vir::Shader::create
        (
            Layer::assembleVertexSource(), 
            Layer::blankFragmentSource_,
            vir::Shader::ConstructFrom::SourceCode
        );
    }
    if (Layer::internalFramebufferShader_ == nullptr)
    {
        std::string fragmentSource = 
version+
R"(out vec4 fragColor;
in vec2 qc;
in vec2 tc;
uniform sampler2D self;
void main(){fragColor = texture(self, tc);})";
        Layer::internalFramebufferShader_ = vir::Shader::create
        (
            Layer::assembleVertexSource(), 
            fragmentSource,
            vir::Shader::ConstructFrom::SourceCode
        );
    }
}

// Public --------------------------------------------------------------------//

void Layer::renderBlankWindow()
{
    const float& aspectRatio = 
        vir::GlobalPtr<vir::Window>::instance()->aspectRatio();
    static float aspectRatio0(0);
    if (Layer::blankQuad_ == nullptr)
        Layer::blankQuad_ = new vir::Quad(1.f, 1.f, 0.f);
    if (aspectRatio != aspectRatio0)
    {
        float width, height;
        if (aspectRatio > 1.0f)
        {
            width = 1.0f;
            height = 1.0f/aspectRatio;
        }
        else
        {
            width = aspectRatio;
            height = 1.0f;
        }
        Layer::blankQuad_->update(width, height, 0.f);
        aspectRatio0 = aspectRatio;
    }
    Layer::createStaticShaders();
    vir::GlobalPtr<vir::Renderer>::instance()->submit
    (
        *Layer::blankQuad_, 
        Layer::blankShader_
    );
}

//----------------------------------------------------------------------------//
//- Non-static members -------------------------------------------------------//

// Public functions ----------------------------------------------------------//

// Constructor/Destructor ----------------------------------------------------//

Layer::Layer
(
    ShaderThingApp& app,
    glm::ivec2 resolution,
    float depth
) :
app_(app),
shader_(nullptr),
rendersTo_(RendersTo::Window),
toBeDeleted_(false),
toBeRenamed_(false),
toBeCompiled_(false),
isGuiRendered_(false),
isGuiDeletionConfirmationPending_(false),
hasUncompiledChanges_(false),
hasHeaderErrors_(false),
isAspectRatioBoundToWindow_(true),
depth_(depth),
resolution_(resolution),
targetResolution_(resolution),
aspectRatio_(float(resolution.x)/float(resolution.y)),
resolutionScale_({1,1}),
viewport_
(
    {
        (resolution.x > resolution.y) ? 1.0 : aspectRatio_,
        (resolution.y > resolution.x) ? 1.0 : 1.0/aspectRatio_
    }
),
fragmentSourceHeader_(""),
fragmentSource_(defaultFragmentSource_),
postProcesses_(0),
screenQuad_(new vir::Quad(viewport_.x, viewport_.y, depth)),
screenCamera_(app.screnCameraRef()),
shaderCamera_(app.shaderCameraRef()),
renderer_(*vir::GlobalPtr<vir::Renderer>::instance()),
shaderId0_(-1),
uncompiledUniforms_(0),
uniformLayerNamesToBeSet_(0),
internalFramebufferClearPolicyOnExport_
(
    InternalFramebufferClearPolicyOnExport::None
)
{
    std::random_device dev;
    std::mt19937 rng(dev());
    std::uniform_int_distribution<std::mt19937::result_type> distribution
    (
        1000,
        1000000000
    );
    id_ = distribution(rng)+(int(app.timeRef())%1000);
    
    initializeEditors();

    // Init default uniforms (must be done before assembleFragmentSource)
    initializeDefaultUniforms();

    // Init shader
    shader_ =
        vir::Shader::create
        (
            Layer::assembleVertexSource(), 
            assembleFragmentSource(fragmentSource_),
            vir::Shader::ConstructFrom::SourceCode
        );

    // Bind shared uniform block
    shader_->bindUniformBlock("KeyboardBlock", app_.keyboardUniformBufferRef(), 0);

    //
    createStaticShaders();

    // Init framebuffers
    flipFramebuffers_ = false;
    framebufferA_ = vir::Framebuffer::create
    (
        resolution_.x, 
        resolution_.y,
        vir::TextureBuffer2D::InternalFormat::RGBA_SF_32
    );
    readOnlyFramebuffer_ = framebufferA_;
    framebufferB_ = vir::Framebuffer::create
    (
        resolution_.x, 
        resolution_.y,
        vir::TextureBuffer2D::InternalFormat::RGBA_SF_32
    );
    writeOnlyFramebuffer_ = framebufferB_;

    ++LayerManager::nLayersSinceNewProject_;
}

//----------------------------------------------------------------------------//

Layer::~Layer()
{
    // Unregister from resource manager
    app_.resourceManagerRef().removeLayerAsResource(this);

    // (Try to) Unregister from quantizer
    //app_.quantizationToolRef().removeLayerAsTarget(this);

    // Delete all managed resources
    for (auto* pp : postProcesses_)
        delete pp;
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
        if (Layer::blankQuad_ != nullptr)
            delete Layer::blankQuad_;
        Layer::blankQuad_ = nullptr;
        if (Layer::blankShader_ != nullptr)
            delete Layer::blankShader_;
        Layer::blankShader_ = nullptr;
        if (Layer::internalFramebufferShader_ != nullptr)
            delete Layer::internalFramebufferShader_;
        Layer::internalFramebufferShader_ = nullptr;
        Layer::sharedSourceEditor_ = ImGuiExtd::TextEditor();
    }
}

//----------------------------------------------------------------------------//

void Layer::render(vir::Framebuffer* target, bool clearTarget)
{
    // Check if shader needs to be compiled
    if (toBeCompiled_)
    {
        compileShader();
        toBeCompiled_ = false;
    }

    // Flip buffers
    flipFramebuffers_ = !flipFramebuffers_;
    writeOnlyFramebuffer_ = flipFramebuffers_?framebufferA_:framebufferB_;
    readOnlyFramebuffer_ = flipFramebuffers_?framebufferB_:framebufferA_;

    // Set all uniforms that are not set in the GUI step
    setSharedDefaultSamplerUniforms();

    // Re-direct rendering & disable blending if not rendering to the window
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (rendersTo_ != RendersTo::Window)
    {
        target = writeOnlyFramebuffer_;
        vir::GlobalPtr<vir::Renderer>::instance()->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    renderer_.submit
    (
        *screenQuad_,
        shader_,
        target,
        clearTarget ||
        rendersTo_ != RendersTo::Window  // Force clear if not
                                         // rendering to window
    );

    // Re-enable blending before either leaving or redirecting the rendered 
    // texture to the main window
    if (!blendingEnabled)
        vir::GlobalPtr<vir::Renderer>::instance()->setBlending(true);

    // Check if layer should be quantized
    // app_.quantizationToolRef().quantize(this);

    // Apply post-processing effects, if any
    for (auto postProcess : postProcesses_)
        postProcess->run();

    if (rendersTo_ != RendersTo::InternalFramebufferAndWindow)
        return;

    // Render texture rendered by the previous call to the provided initial 
    // target (or to main window if target0 == nullptr)
    Layer::internalFramebufferShader_->bind();
    writeOnlyFramebuffer_->bindColorBuffer(0);
    Layer::internalFramebufferShader_->setUniformInt("self", 0);
    renderer_.submit
    (
        *screenQuad_, 
        Layer::internalFramebufferShader_, 
        target0,
        clearTarget
    );
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

    if (fragmentSourceEditor_.IsTextChanged())
        hasUncompiledChanges_ = 
            fragmentSource_!=fragmentSourceEditor_.GetText();

    if (uncompiledUniforms_.size() > 0)
    {
        bool allUniformsAreNamed = true;
        for (auto* uniform : uncompiledUniforms_)
        {
            if (uniform->name.size()>0)
                continue;
            allUniformsAreNamed = false;
            break;
        }
        if (allUniformsAreNamed)
            hasUncompiledChanges_ = true;
    }

    if (resolution_ == targetResolution_)
        return;
    resolution_ = targetResolution_;
    aspectRatio_ = float(resolution_.x)/float(resolution_.y);
    app_.exportToolRef().updateLayerResolutions();

    // Regardless of this layer's resolution, graphically, the layer quad needs
    // to fit the main window size, hence why the window aspect ratio is used
    // for setting the quad size
    float windowAspectRatio = 
        vir::GlobalPtr<vir::Window>::instance()->aspectRatio();
    viewport_.x = std::min(1.0f, windowAspectRatio);
    viewport_.y = std::min(1.0f, 1.0f/windowAspectRatio);
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

    // Lightweight version of render restart
    app_.frameRef() = -1;
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
            u->type != Uniform::Type::Sampler2D && 
            u->type != Uniform::Type::SamplerCube
        )
            continue;
        auto r = u->getValuePtr<Resource>();
        if (r == nullptr)
            continue;
        if (r->name() != resource->name())
            continue;
        uniforms_.erase(uniforms_.begin()+index);
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

// De-/serialization functions -----------------------------------------------//

void Layer::saveState(ObjectIO& writer)
{
    writer.writeObjectStart(name_.c_str());
    writer.write("renderTarget", (int)rendersTo_ );
    writer.write("resolution", resolution_);
    writer.write("resolutionScale", resolutionScale_);
    writer.write("depth", depth_);

    writer.writeObjectStart("internalFramebuffer");
    writer.write("format", (int)readOnlyFramebuffer_->
        colorBufferInternalFormat());
    writer.write("wrapModes", glm::ivec2(
        (int)readOnlyFramebuffer_->colorBufferWrapMode(0),
        (int)readOnlyFramebuffer_->colorBufferWrapMode(1)));
    writer.write("magnificationFilterMode", 
        (int)readOnlyFramebuffer_->colorBufferMagFilterMode());
    writer.write("minimizationFilterMode",
        (int)readOnlyFramebuffer_->colorBufferMinFilterMode());
    writer.write("exportClearPolicy", 
        (int)internalFramebufferClearPolicyOnExport_);
    writer.writeObjectEnd(); // End of internalFramebuffer

    writer.writeObjectStart("shader");
    writer.write("fragmentSource", fragmentSource_.c_str(), 
        fragmentSource_.size(), true);
    
    writer.writeObjectStart("uniforms");
    for (auto u : uniforms_)
    {
        float& min(u->limits.x);
        float& max(u->limits.y);
        if (u->name.size() == 0)
            continue;
        writer.writeObjectStart(u->name.c_str());
        writer.write("type", vir::Shader::uniformTypeToName[u->type].c_str());

#define WRITE_MIN_MAX               \
        writer.write("min", min);   \
        writer.write("max", max);   \

        switch(u->type)
        {
            case vir::Shader::Variable::Type::Bool :
            {
                writer.write("value", u->getValue<bool>());
                break;
            }
            case vir::Shader::Variable::Type::Int :
            {
                writer.write("value", u->getValue<int>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Int2 :
            {
                writer.write("value", u->getValue<glm::ivec2>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Int3 :
            {
                writer.write("value", u->getValue<glm::ivec3>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Int4 :
            {
                writer.write("value", u->getValue<glm::ivec4>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Float :
            {
                writer.write("value", u->getValue<float>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Float2 :
            {
                writer.write("value", u->getValue<glm::vec2>());
                WRITE_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Float3 :
            {
                writer.write("value", u->getValue<glm::vec3>());
                WRITE_MIN_MAX
                writer.write("usesColorPicker", u->usesColorPicker);
                break;
            }
            case vir::Shader::Variable::Type::Float4 :
            {
                writer.write("value", u->getValue<glm::vec4>());
                WRITE_MIN_MAX
                writer.write("usesColorPicker", u->usesColorPicker);
            }
            case vir::Shader::Variable::Type::Sampler2D :
            case vir::Shader::Variable::Type::SamplerCube :
            {
                auto r = u->getValuePtr<Resource>();
                writer.write("value", r->name().c_str());
                break;
            }
            default:
                break;
        }
        writer.writeObjectEnd(); // End of 'u->name'
    }
    writer.writeObjectEnd(); // End of uniforms
    writer.writeObjectEnd(); // End of shaders

    // Write post-processing effects data, if any
    if (postProcesses_.size() > 0)
    {
        writer.writeObjectStart("postProcesses");
        for (auto postProcess : postProcesses_)
            postProcess->saveState(writer);
        writer.writeObjectEnd(); // End of postProcesses
    }
    writer.writeObjectEnd(); // End of 'name_'
}

//----------------------------------------------------------------------------//

void Layer::rebindLayerUniforms()
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

std::string Layer::assembleFragmentSource
(
    const std::string& source,
    int* nHeaderLines
)
{
    int nLines = 4;
    static std::string versionInOutHeader
    (
        "#version "+
        vir::GlobalPtr<vir::Window>::instance()->context()->
        shadingLanguageVersion()+
        " core\nin vec2 qc;\nin vec2 tc;\nout vec4 fragColor;\n"
    );
    fragmentSourceHeader_ = versionInOutHeader;
    for (auto* u : app_.sharedUniformsRef())
    {
        if (u->specialType == Uniform::SpecialType::Keyboard)
        {
            fragmentSourceHeader_ +=
                "layout (std140) uniform KeyboardBlock{ivec3 "+u->name+
                "[256];};\n";
        }
        else
        {
            fragmentSourceHeader_ += 
                "uniform "+vir::Shader::uniformTypeToName[u->type]+" "+u->name+
                ";\n";
        }
        ++nLines;
    }

    for (auto* u : defaultUniforms_)
    {
        fragmentSourceHeader_ += 
            "uniform "+vir::Shader::uniformTypeToName[u->type]+" "+u->name+
            ";\n";
        ++nLines;
    }
    for (auto* u : uniforms_)
    {
        // If the uniform has no name, I can't add it to the source
        if (u->name.size() == 0)
            continue;
        // All names in uniformTypeToName map 1:1 to GLSL uniform names, except
        // for sampler2D (which maps to texture2D) and samplerCube (which maps
        // to cubemap). I should re-organize the mappings a bit
        std::string typeName; 
        bool isSampler2D(false);
        switch (u->type)
        {
            case vir::Shader::Variable::Type::Sampler2D :
                isSampler2D = true;
                typeName = "sampler2D";
                break;
            case vir::Shader::Variable::Type::SamplerCube :
                typeName = "samplerCube";
                break;
            default :
                typeName = vir::Shader::uniformTypeToName[u->type];
                break;
        }
        fragmentSourceHeader_ += "uniform "+typeName+" "+u->name+";\n";
        ++nLines;
        // Automatically managed sampler2D resolution uniform
        if (isSampler2D)
        {
            fragmentSourceHeader_ += "uniform vec2 "+u->name+"Resolution;\n";
            ++nLines;
        }
    }
    int nSharedLines = Layer::sharedSourceEditor_.GetTotalLines();
    if (nHeaderLines != nullptr)
        *nHeaderLines = nLines+nSharedLines;
    return 
        fragmentSourceHeader_+
        Layer::sharedSourceEditor_.GetText()+"\n"+
        source;
}

//----------------------------------------------------------------------------//

bool Layer::compileShader()
{
    if (!hasUncompiledChanges_)
        return true;

    fragmentSource_ = fragmentSourceEditor_.GetText();

    vir::Shader* tmp = nullptr;
    std::exception_ptr exceptionPtr;
    int nHeaderLines = 0;
    tmp = vir::Shader::create
    (
        assembleVertexSource(),
        assembleFragmentSource(fragmentSource_, &nHeaderLines), 
        vir::Shader::ConstructFrom::SourceCode,
        &exceptionPtr
    );
    hasHeaderErrors_ = false;
    sharedSourceHasErrors_ = false;
    if (tmp != nullptr)
    {
        app_.frameRef() = 0;
        delete shader_;
        shader_ = tmp;
        tmp = nullptr;
        sharedSourceEditor_.SetErrorMarkers({});
        fragmentSourceEditor_.SetErrorMarkers({});
        // Remove only named uniforms form the list of the uncompiled uniforms
        // as they are the only ones that get compiled into the source code
        uncompiledUniforms_.erase
        (
            std::remove_if
            (
                uncompiledUniforms_.begin(),
                uncompiledUniforms_.end(),
                [](Uniform* u){return u->name.size()>0;}
            ),
            uncompiledUniforms_.end()
        );
        hasUncompiledChanges_ = false;
        return true;
    }
    try {std::rethrow_exception(exceptionPtr);}
    catch(std::exception& e)
    {
        // If the compilation fails, parse the error and show it
        // on the editor
        hasUncompiledChanges_ = true;
        std::string exception(e.what());
        //std::cout << exception << std::endl;
        bool isFragmentException = false;
        if (exception[0] == '[' && exception[1] == 'F' &&
            exception[2] == ']')
            isFragmentException = true;
        if (!isFragmentException)
            return false;
        bool readErrorIndex = true;
        bool sharedError = false;
        int firstErrorIndex = -1;
        int errorIndex = 0;
        std::string errorIndexString = "";
        std::string error = "";
        ImGuiExtd::TextEditor::ErrorMarkers errors, sharedErrors;
        int i = 3;
        int n = exception.size();
        while(i < n)
        {
            char ei(exception[i]);
            char eip1(exception[std::min(n-1, i+1)]);
            if (readErrorIndex && (ei == '0' && (eip1 == '(' || eip1 == ':')))
            {
                i += 2;
                while(exception[i] != ')' && exception[i] != ':')
                    errorIndexString += exception[i++];
                errorIndex = std::stoi(errorIndexString)-nHeaderLines;
                sharedError = false;
                if (errorIndex < 1)
                {
                    int nSharedLines = 
                        Layer::sharedSourceEditor_.GetTotalLines();
                    if (errorIndex > -nSharedLines)
                    {
                        sharedError = true;
                        sharedSourceHasErrors_ = true;
                        errorIndex += nSharedLines;
                    }
                    else
                    {
                        hasHeaderErrors_ = true;
                        errorIndex = -1;
                    }
                }
                errorIndexString = "";
                if (firstErrorIndex == -1)
                    firstErrorIndex = errorIndex;
                readErrorIndex = false;
                ++i;
                while (exception[i] == ' ' || exception[i] == ':')
                    ++i;
                --i;
            }
            else if (firstErrorIndex != -1)
            {
                if (ei != '\n')
                    error += ei;
                else
                {
                    if (sharedError)
                        sharedErrors.insert({errorIndex, error});
                    else
                        errors.insert({errorIndex+1, error});
                    error = "";
                    readErrorIndex = true;
                }
            }
            ++i;
        }
        if (errors.size() > 0)
        {
            fragmentSourceEditor_.SetErrorMarkers(errors);
            auto pos = ImGuiExtd::TextEditor::Coordinates(firstErrorIndex,0);
            fragmentSourceEditor_.SetCursorPosition(pos);
        }
        if (sharedErrors.size() > 0)
        {
            sharedSourceEditor_.SetErrorMarkers(sharedErrors);
        }
        return false;
    }
}

//----------------------------------------------------------------------------//

void Layer::initializeEditors()
{
    fragmentSourceEditor_.SetText(fragmentSource_);
    fragmentSourceEditor_.SetLanguageDefinition
    (
        ImGuiExtd::TextEditor::LanguageDefinition::GLSL()
    );
    fragmentSourceEditor_.ResetTextChanged();
    if (Layer::sharedSourceEditor_.GetText().size() > 0)
        return;
    Layer::sharedSourceEditor_.SetLanguageDefinition
    (
        ImGuiExtd::TextEditor::LanguageDefinition::GLSL()
    );
    Layer::sharedSourceEditor_.SetText(Layer::defaultSharedSource_);
    Layer::sharedSourceEditor_.ResetTextChanged();
}

//----------------------------------------------------------------------------//

void Layer::initializeDefaultUniforms()
{
    // iUserAction
    // Flag-uniform to signal that the user has changed inputs/uniforms. It is
    // not shared per-se but does react to changes in shared uniforms as well
    auto userActionUniform = new Uniform();
    userActionUniform->specialType = Uniform::SpecialType::UserAction;
    userActionUniform->name = "iUserAction";
    userActionUniform->type = Uniform::Type::Bool;
    userActionUniform->setValuePtr(&app_.userActionRef());
    userActionUniform->showLimits = false;
    defaultUniforms_.emplace_back(userActionUniform);

    // iAspectRatio (of this layer, i.e., ratio of resolution x/y. This does not
    // necessarily have to correspond to the window aspect ratio, even though
    // graphically all layer quads are ultimately squeezed/stretched so to 
    // fit the window aspect ratio exactly)
    auto aspectRatio = new Uniform();
    aspectRatio->specialType = Uniform::SpecialType::LayerAspectRatio;
    aspectRatio->name = "iAspectRatio";
    aspectRatio->type = Uniform::Type::Float;
    aspectRatio->setValuePtr(&aspectRatio_);
    aspectRatio->showLimits = false;
    defaultUniforms_.emplace_back(aspectRatio);

    // iResolution (of this layer, the main window resolution is handled by
    // the iWindowResolution, a shared uniform managed by the top-level app)
    auto resolutionUniform = new Uniform();
    resolutionUniform->specialType = Uniform::SpecialType::LayerResolution;
    resolutionUniform->name = "iResolution";
    resolutionUniform->type = Uniform::Type::Float2;
    resolutionUniform->setValuePtr(&targetResolution_);
    resolutionUniform->limits = glm::vec2(1.0f, 4096.0f);
    resolutionUniform->showLimits = false;
    defaultUniforms_.emplace_back(resolutionUniform);
}

//----------------------------------------------------------------------------//

// This whole function could be re-written in a more elegant way, so to leverage
// the fact that I already have all the shared and default uniforms (i.e., their
// names and values) in app_.sharedUniformsRef() and defaultUniforms_
// respectively
void Layer::setSharedDefaultSamplerUniforms()
{
    // Check for shader recompilation (then, uniforms need to be
    // reset)
    const glm::mat4& mvp(screenCamera_.projectionViewMatrix());
    const glm::vec3& cameraPosition(shaderCamera_.position());
    const glm::vec3& cameraDirection(shaderCamera_.z());
    const glm::ivec4& mouse(app_.mouseRef());
    shader_->bind();

    // Set shared and default uniforms
    bool forceSet((int)shader_->id() != shaderId0_);
    shader_->setUniformFloat("iTime", app_.timeRef());
    shader_->setUniformUInt("iFrame", app_.frameRef());
    shader_->setUniformUInt("iRenderPass", app_.renderPassRef());
    shader_->setUniformFloat
    (
        "iWindowAspectRatio", 
        vir::GlobalPtr<vir::Window>::instance()->aspectRatio()
    );
    shader_->setUniformFloat2("iWindowResolution", app_.resolutionRef());
    if (mvp0_ != mvp || forceSet)
    {
        shader_->setUniformMat4("mvp", mvp);
        Layer::blankShader_->bind();
        Layer::blankShader_->setUniformMat4("mvp", mvp);
        Layer::internalFramebufferShader_->bind();
        Layer::internalFramebufferShader_->setUniformMat4("mvp", mvp);
        shader_->bind();
        mvp0_ = mvp;
    }
    if (resolution0_ != resolution_ || forceSet)
    {
        shader_->setUniformFloat("iAspectRatio", aspectRatio_);
        shader_->setUniformFloat2("iResolution", resolution_);
        resolution0_ = resolution_;
        app_.userActionRef() = true;
    }
    if (cameraPosition0_ != shaderCamera_.position()  || forceSet)
    {
        shader_->setUniformFloat3("iWASD", cameraPosition);
        cameraPosition0_ = cameraPosition;
        app_.userActionRef() = true;
    }
    if (cameraDirection0_ != cameraDirection || forceSet)
    {
        shader_->setUniformFloat3("iLook", cameraDirection);
        cameraDirection0_ = cameraDirection;
        app_.userActionRef() = true;
    }
    if (mouse0_ != mouse || forceSet)
    {
        shader_->setUniformInt4("iMouse", mouse);
        mouse0_ = mouse;
        app_.userActionRef() = true;
    }
    if (userAction0_ != app_.userActionRef())
    {
        shader_->setUniformBool("iUserAction", app_.userActionRef());
        userAction0_ = app_.userActionRef();
    }

    // Set sampler and keyboard-input-bound uniforms
    uint32_t unit = 0; 
    for (auto u : uniforms_)
    {
        bool named(u->name.size() > 0);
        if 
        (
            !named || 
            (
                u->type != vir::Shader::Variable::Type::Sampler2D &&
                u->type != vir::Shader::Variable::Type::SamplerCube
            )
        )
            continue;
        
        // Sampler case
        auto resource = u->getValuePtr<Resource>();
        if (resource == nullptr)
            continue;
        // These two lines are required when returning writeOnlyFramebuffer_
        // in readOnlyFramebuffer() to avoid visual artifacts. Depending on
        // the outcome of my experiments, I might keep this approach
        if (resource->namePtr() == &name_)
        {
            vir::Framebuffer* sourceFramebuffer = readOnlyFramebuffer_;
            for (auto* postProcess : postProcesses_)
            {
                if 
                (
                    postProcess->isActive() && 
                    postProcess->outputFramebuffer() != nullptr
                )
                    sourceFramebuffer = postProcess->outputFramebuffer();
            }
            sourceFramebuffer->bindColorBuffer(unit);
        }
        else
            resource->bind(unit);
        shader_->setUniformInt(u->name, unit);
        unit++;
        // Set the (automatically managed) sampler2D resolution uniform
        if (u->type == vir::Shader::Variable::Type::Sampler2D)
        {
            shader_->setUniformFloat2
            (
                u->name+"Resolution", 
                {
                    resource->width(),
                    resource->height()
                }
            );
        }
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

#define CASE(ST, T, F)                                      \
case vir::Shader::Variable::Type::ST :                      \
{                                                           \
    T value = u->getValue<T>();                             \
    u->setValue(value);                                     \
    if (named)                                              \
        shader_->F(u->name, value);                         \
    break;                                                  \
}   
    for (auto u : uniforms_)
    {
        bool named(u->name.size() > 0);
        switch(u->type)
        {
            CASE(Bool, bool, setUniformBool)
            CASE(Int, int, setUniformInt)
            CASE(Int2, glm::ivec2, setUniformInt2)
            CASE(Int3, glm::ivec3, setUniformInt3)
            CASE(Int4, glm::ivec4, setUniformInt4)
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
    glm::ivec2 value0 = resolution_;
    glm::ivec2& value(targetResolution_);
    auto window = 
        vir::GlobalPtr<vir::Window>::instance();
    static const float& aspectRatio
    (
        window->aspectRatio()
    );
    ;
    static glm::ivec2 maxResolution;
    if (rendersTo_ != RendersTo::Window)
    {
        static glm::ivec2 minResolution({1,1});
        static glm::ivec2 maxResolution({4096,4096});
        if (isAspectRatioBoundToWindow_)
        {
            if (value0.x != value.x)
                value.y = (float)value.x/aspectRatio+.5f;
            else if (value0.y != value.y)
                value.x = (float)value.y*aspectRatio+.5f;
        }
        value.x = std::max(minResolution.x, value.x);
        value.x = std::min(maxResolution.x, value.x);
        value.y = std::max(minResolution.y, value.y);
        value.y = std::min(maxResolution.y, value.y);
    }
    else
        Misc::limitWindowResolution(value);
    
    if (rendersTo_ != RendersTo::Window)
    {
        if (isAspectRatioBoundToWindow_)
        {
            if (value.x > value.y)
            {
                resolutionScale_.x = 
                    (float)value.x/window->width();
                resolutionScale_.y = resolutionScale_.x;
            }
            else
            {
                resolutionScale_.y = 
                    (float)value.y/window->height();
                resolutionScale_.x = resolutionScale_.y;
            }
        }
        else
        {
            resolutionScale_.x = 
                (float)value.x/window->width();
            resolutionScale_.y = 
                (float)value.y/window->height();
        }
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

//----------------------------------------------------------------------------//

void Layer::clearFramebuffers()
{
   framebufferA_->clearColorBuffer();
   framebufferB_->clearColorBuffer();
}

//----------------------------------------------------------------------------//

void Layer::clearFramebuffersWithPolicy()
{
    switch (internalFramebufferClearPolicyOnExport_)
    {
    case InternalFramebufferClearPolicyOnExport::None :
        return;
    case InternalFramebufferClearPolicyOnExport::ClearOnFirstFrame :
        if (app_.isExportingAndFirstFrame())
            clearFramebuffers();
        return;
    case InternalFramebufferClearPolicyOnExport::ClearOnEveryFrame :
        if (app_.isExportingAndFirstRenderPassInFrame())
            clearFramebuffers();
        return;
    }
}

//----------------------------------------------------------------------------//

float Layer::aspectRatio() const
{
    if (isAspectRatioBoundToWindow_)
        return vir::GlobalPtr<vir::Window>::instance()->aspectRatio();
    return aspectRatio_;
}

//----------------------------------------------------------------------------//

Layer::Layer
(
    ShaderThingApp& app,
    const ObjectIO& reader,
    bool isGuiRendered
) :
app_(app),
shader_(nullptr),
rendersTo_(RendersTo::Window),
toBeDeleted_(false),
toBeRenamed_(false),
isGuiRendered_(isGuiRendered),
isGuiDeletionConfirmationPending_(false),
toBeCompiled_(false),
fragmentSourceHeader_(""),
hasUncompiledChanges_(false),
hasHeaderErrors_(false),
isAspectRatioBoundToWindow_(true),
postProcesses_(0),
screenCamera_(app.screnCameraRef()),
shaderCamera_(app.shaderCameraRef()),
renderer_(*vir::GlobalPtr<vir::Renderer>::instance()),
shaderId0_(-1),
uncompiledUniforms_(0),
uniformLayerNamesToBeSet_(0),
internalFramebufferClearPolicyOnExport_
(
    InternalFramebufferClearPolicyOnExport::None
)
{
    // Name & id
    name_ = reader.name();
    targetName_ = name_;
    std::random_device dev;
    std::mt19937 rng(dev());
    id_ = std::uniform_int_distribution<std::mt19937::result_type>(1e3,
        1e9)(rng);
    
    // Geometric and rendering parameters
    rendersTo_ = (RendersTo)reader.read<int>("renderTarget");
    depth_ = reader.read<float>("depth");
    resolution_ = reader.read<glm::ivec2>("resolution");
    targetResolution_ = resolution_;
    aspectRatio_ = float(resolution_.x)/float(resolution_.y);
    resolutionScale_ = reader.read<glm::vec2>("resolutionScale");
    isAspectRatioBoundToWindow_ = (resolutionScale_.x == resolutionScale_.y);
    viewport_.x = (resolution_.x > resolution_.y) ? 1.0 : aspectRatio_;
    viewport_.y = (resolution_.y > resolution_.x) ? 1.0 : 1.0/aspectRatio_;
    
    //
    screenQuad_ = new vir::Quad(viewport_.x, viewport_.y, depth_);

    auto shaderData = reader.readObject("shader");
    fragmentSource_ = shaderData.read("fragmentSource", false);

    auto uniformsData = shaderData.readObject("uniforms");
    for(auto uniformName : uniformsData.members())
    {
        auto uniformData = uniformsData.readObject(uniformName);
        auto uniform = new Uniform();
        uniform->type = vir::Shader::uniformNameToType[
            uniformData.read<std::string>("type")];
        uniforms_.emplace_back(uniform);
        float min, max, x, y, z, w;

#define SET_UNIFORM(type)                   \
    uniform->setValue<type>(uniformData.read<type>("value"));
#define READ_MIN_MAX                        \
    min = uniformData.read<float>("min");   \
    max = uniformData.read<float>("max");

        switch (uniform->type)
        {
            case vir::Shader::Variable::Type::Bool :
            {
                SET_UNIFORM(bool)
                uniform->showLimits = false;
                break;
            }
            case vir::Shader::Variable::Type::Int :
            {
                SET_UNIFORM(int)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Int2 :
            {
                SET_UNIFORM(glm::ivec2)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Int3 :
            {
                SET_UNIFORM(glm::ivec3)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Int4 :
            {
                SET_UNIFORM(glm::ivec4)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Float :
            {
                SET_UNIFORM(float)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Float2 :
            {
                SET_UNIFORM(glm::vec2)
                READ_MIN_MAX
                break;
            }
            case vir::Shader::Variable::Type::Float3 :
            {
                SET_UNIFORM(glm::vec3)
                READ_MIN_MAX
                uniform->usesColorPicker = uniformData.read<bool>(
                    "usesColorPicker");
                uniform->showLimits = !uniform->usesColorPicker;
                break;
            }
            case vir::Shader::Variable::Type::Float4 :
            {
                SET_UNIFORM(glm::vec4)
                READ_MIN_MAX
                uniform->usesColorPicker = uniformData.read<bool>(
                    "usesColorPicker");
                uniform->showLimits = !uniform->usesColorPicker;
                break;
            }
            case vir::Shader::Variable::Type::Sampler2D :
            case vir::Shader::Variable::Type::SamplerCube :
            {
                std::string resourceName = uniformData.read("value", false);
                uniform->showLimits = false;
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
        uniform->limits = {min, max};
        uniform->name = uniformName;
    }
    
    initializeEditors();

    // Init default uniforms (must be done before assembleFragmentSource)
    initializeDefaultUniforms();

    //
    createStaticShaders();

    // Init shader
    // If the project was saved in a state such that the shader has compilation
    // errors, then initialize the shader with the blank shader source (back-end
    // -only, the user will still see the source of the saved shader with the 
    // usuale list of compilation errors and markers)
    hasUncompiledChanges_ = true; // Set to true to force compilation
    if (!compileShader())
        shader_ = 
            vir::Shader::create
            (
                Layer::assembleVertexSource(),
                Layer::blankFragmentSource_,
                vir::Shader::ConstructFrom::SourceCode
            );

    auto framebufferData = reader.readObject("internalFramebuffer");
    auto internalFormat = 
        (vir::TextureBuffer::InternalFormat)framebufferData.read<int>("format");

    // Init framebuffers
    flipFramebuffers_ = false;
    framebufferA_ = vir::Framebuffer::create
    (
        resolution_.x, 
        resolution_.y, 
        internalFormat
    );
    readOnlyFramebuffer_ = framebufferA_;
    framebufferB_ = vir::Framebuffer::create
    (
        resolution_.x, 
        resolution_.y, 
        internalFormat
    );
    writeOnlyFramebuffer_ = framebufferB_;

    // Set framebuffer color attachment wrapping and filtering settings
    auto magFilter = framebufferData.read<int>("magnificationFilterMode");
    auto minFilter = framebufferData.read<int>("minimizationFilterMode");
    auto wrapModes = framebufferData.read<glm::ivec2>("wrapModes");
    auto setWrapFilterSettings = [&](vir::Framebuffer* framebuffer)->void
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
            (vir::TextureBuffer::WrapMode)wrapModes.x
        );
        framebuffer->setColorBufferWrapMode
        (
            1,
            (vir::TextureBuffer::WrapMode)wrapModes.y
        );
    };
    setWrapFilterSettings(framebufferA_);
    setWrapFilterSettings(framebufferB_);

    // Initialize post-processing effects, if any were saved
    if (reader.hasMember("postProcesses"))
    {
        auto postProcessData = reader.readObject("postProcesses");
        for (auto name : postProcessData.members())
        {
            ObjectIO data(postProcessData.readObject(name));
            postProcesses_.emplace_back
            (
                PostProcess::create(app_, this, data)
            );
        }
    }

    //
    if (rendersTo_ != RendersTo::Window)
        app.resourceManagerRef().addLayerAsResource(this);

    ++LayerManager::nLayersSinceNewProject_;
}

void Layer::setFramebufferWrapMode(int i, vir::TextureBuffer::WrapMode mode)
{
    framebufferA_->setColorBufferWrapMode(i, mode);
    framebufferB_->setColorBufferWrapMode(i, mode);
}

void Layer::setFramebufferMagFilterMode(vir::TextureBuffer::FilterMode mode)
{
    framebufferA_->setColorBufferMagFilterMode(mode);
    framebufferB_->setColorBufferMagFilterMode(mode);
}

void Layer::setFramebufferMinFilterMode(vir::TextureBuffer::FilterMode mode)
{
    framebufferA_->setColorBufferMinFilterMode(mode);
    framebufferB_->setColorBufferMinFilterMode(mode);
}

}
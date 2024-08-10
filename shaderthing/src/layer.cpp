/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthing/include/layer.h"

#include "shaderthing/include/helpers.h"
#include "shaderthing/include/macros.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/postprocess.h"
#include "shaderthing/include/resource.h"
#include "shaderthing/include/sharedstorage.h"
#include "shaderthing/include/shareduniforms.h"
#include "shaderthing/include/uniform.h"

#include "vir/include/vir.h"

#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

bool        Layer::Flags::restartRendering  = false;
std::string Layer::GUI::defaultSharedSource = 
R"(// Common source code is shared by all fragment shaders across all layers and
// has access to all shared in/out/uniform declarations

#define IF_FRAG_X(X) if (int(gl_FragCoord.x)==X)
#define IF_FRAG_Y(Y) if (int(gl_FragCoord.y)==Y)
#define IF_FRAG_XY(X,Y) if (int(gl_FragCoord.x)==X && int(gl_FragCoord.y)==Y)

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
#define SHADERTOY_MAIN void main(){mainImage(fragColor, fragCoord);}
vec2 fragCoord = gl_FragCoord.xy;

#define CROSSHAIR(color)                           \
    if(int(gl_FragCoord.x)==int(iResolution.x/2)|| \
       int(gl_FragCoord.y)==int(iResolution.y/2))  \
       fragColor.rgb=color;
)";

TextEditor Layer::GUI::sharedSourceEditor = 
    TextEditor(Layer::GUI::defaultSharedSource);

// Shader for mapping the contents of a framebuffer to another one, potentially
// at a different resolution and/or internal format
std::unique_ptr<vir::Shader> Layer::Rendering::textureMapperShader = nullptr;

// Shared storage buffer for all layers
std::unique_ptr<SharedStorage> Layer::Rendering::sharedStorage     = nullptr;

//----------------------------------------------------------------------------//

Layer::Layer
(
    const std::vector<Layer*>& layers,
    const SharedUniforms& sharedUniforms
) :
    id_(findFreeId(layers))
{
    setResolution(sharedUniforms.iResolution(), false);

    // Add default uniforms
    {
        Uniform* u = nullptr;
        
        u = new Uniform{};
        u->specialType = Uniform::SpecialType::LayerAspectRatio;
        u->name = "iAspectRatio";
        u->type = Uniform::Type::Float;
        u->setValuePtr(&aspectRatio_);
        u->gui.showBounds = false;
        uniforms_.emplace_back(u);

        u = new Uniform{};
        u->specialType = Uniform::SpecialType::LayerResolution;
        u->name = "iResolution";
        u->type = Uniform::Type::Float2;
        u->setValuePtr(&resolution_);
        u->gui.bounds = glm::vec2(1.0f, 4096.0f);
        u->gui.showBounds = false;
        uniforms_.emplace_back(u);
    };

    setName("Layer "+std::to_string(id_));

    // Set default fragment source in editor
    gui_.sourceEditor.setText
    (
R"(void main()
{
/*  Quick description of some important shader inputs and uniforms:

    >>  qc (quad coordinates) represents the coordinates of the current pixel
        (i.e., fragment) in a Euclidian reference frame with the origin at the 
        window center. The magnitude of qc varies from -0.5 to 0.5 along the 
        longest side of the window
    
    >>  tc (texture coordinates) represents the coordinates of the current pixel
        (i.e., fragment) in an affine reference frame with the origin at the
        window bottom-left corner, and where (1, 1) is always at the window top-
        right corner, regardless of the current window size or aspect ratio

    >>  iTime is the elapsed wall time. It can be modified in the 'Uniforms' tab
    
    >>  for a full list of all available uniforms, expand the shader 'Header'
        at the top of the source code. This is inclusive of user-created 
        uniforms in the 'Uniforms' tab*/

    // Output pixel color (all components are in the [0, 1] range)
    fragColor = vec4
    ( 
        .4+.250*sin(2.*(qc.x+iTime)), // Red
        .5+.125*cos(3.*(tc.y+iTime)), // Green
        .75,                          // Blue
        1.                            // Alpha (transparency)
    );
})"
    );
    gui_.sourceEditor.resetTextChanged();

    // Initialize shared storage - needs to be done before shader compilation
    if (Rendering::sharedStorage == nullptr)
        Rendering::sharedStorage = std::make_unique<SharedStorage>();

    // Compile shader
    compileShader(sharedUniforms);

    // Set depth (also inits rendering quad on first call)
    setDepth((float)layers.size()/Layer::nMaxLayers);

    // Register with event broadcaster
    this->tuneIntoEventBroadcaster(VIR_DEFAULT_PRIORITY+id_);

    // Initialize std::unique_ptrs to manage static shaders
    if (Layer::Rendering::textureMapperShader != nullptr)
        return;
    Layer::Rendering::textureMapperShader = std::unique_ptr<vir::Shader>
    (
        vir::Shader::create
        (
            vertexShaderSource(sharedUniforms),
            glslVersionSource()+
R"(out  vec4      fragColor;
in      vec2      qc;
in      vec2      tc;
uniform sampler2D tx;
void main(){fragColor = texture(tx, tc);})",
            vir::Shader::ConstructFrom::SourceCode
        )
    );
    sharedUniforms.bindShader(Layer::Rendering::textureMapperShader.get());
}

//----------------------------------------------------------------------------//

Layer::~Layer()
{
    DELETE_IF_NOT_NULLPTR(rendering_.framebufferA)
    DELETE_IF_NOT_NULLPTR(rendering_.framebufferB)
    DELETE_IF_NOT_NULLPTR(rendering_.shader)
    DELETE_IF_NOT_NULLPTR(rendering_.quad)
    for (auto postProcess : rendering_.postProcesses)
    {
        DELETE_IF_NOT_NULLPTR(postProcess)
    }
}

//----------------------------------------------------------------------------//

void Layer::save(ObjectIO& io) const
{
    io.writeObjectStart(gui_.name.c_str());
    io.write("renderTarget", (int)rendering_.target);
    io.write("resolution", resolution_);
    io.write("resolutionRatio", resolutionRatio_);
    io.write("isAspectRatioBoundToWindow", flags_.isAspectRatioBoundToWindow);
    io.write("rescaleWithWindow", flags_.rescaleWithWindow);
    io.write("depth", depth_);

    io.writeObjectStart("internalFramebuffer");
    auto framebuffer = rendering_.framebuffer;
    io.write("format", (int)framebuffer->colorBufferInternalFormat());
    io.write
    (
        "wrapModes", 
        glm::ivec2
        (
            (int)framebuffer->colorBufferWrapMode(0),
            (int)framebuffer->colorBufferWrapMode(1)
        )
    );
    io.write
    (
        "magFilterMode", 
        (int)framebuffer->colorBufferMagFilterMode()
    );
    io.write
    (
        "minFilterMode",
        (int)framebuffer->colorBufferMinFilterMode()
    );
    io.write("exportClearPolicy", (int)exportData_.clearPolicy);
    io.writeObjectEnd(); // End of internalFramebuffer

    io.writeObjectStart("exportData");
    io.write("resolutionScale", exportData_.resolutionScale);
    io.write("rescaleWithOutput", exportData_.rescaleWithOutput);
    io.write("windowResolutionScale", exportData_.windowResolutionScale);
    io.writeObjectEnd(); // End of exportData

    io.writeObjectStart("shader");
    auto fragmentSource = gui_.sourceEditor.getText();
    io.write
    (
        "fragmentSource",
        fragmentSource.c_str(),
        fragmentSource.size(),
        true
    );
    
    Uniform::saveAll(io, uniforms_);
    
    io.writeObjectEnd(); // End of shaders

    // Write post-processing effects data, if any
    if (rendering_.postProcesses.size() > 0)
    {
        io.writeObjectStart("postProcesses");
        for (auto postProcess : rendering_.postProcesses)
            postProcess->save(io);
        io.writeObjectEnd(); // End of postProcesses
    }
    
    io.writeObjectEnd(); // End of 'gui_.name'
}

//----------------------------------------------------------------------------//

void Layer::save(const std::vector<Layer*>& layers, ObjectIO& io)
{
    auto sharedSource = Layer::GUI::sharedSourceEditor.getText();
    if (Layer::GUI::defaultSharedSource != sharedSource)
        io.write
        (
            "sharedFragmentSource", 
            sharedSource.c_str(), 
            sharedSource.size(), 
            true
        );
    Layer::Rendering::sharedStorage->save(io);
    io.writeObjectStart("layers");
    for (auto layer : layers)
        layer->save(io);
    io.writeObjectEnd();
}

//----------------------------------------------------------------------------//

Layer* Layer::load
(
    const ObjectIO& io,
    const std::vector<Layer*>& layers,
    const SharedUniforms& sharedUniforms,
    std::vector<Resource*>& resources
)
{
    auto layer = new Layer(layers, sharedUniforms);

    layer->setName(io.name());
    layer->flags_.rename = true; // <- hack to prevent layer tab bar re-ordering
                                 // on first renderGui after loading
    layer->rendering_.target = (Rendering::Target)io.read<int>("renderTarget");
    layer->resolution_ = io.read<glm::ivec2>("resolution");
    layer->aspectRatio_ = float(layer->resolution_.x)/layer->resolution_.y;
    layer->resolutionRatio_ = io.read<glm::vec2>("resolutionRatio");
    layer->flags_.rescaleWithWindow = 
        io.readOrDefault<bool>("rescaleWithWindow", true);
    
    layer->setDepth(io.read<float>("depth"));

    auto exportData = io.readObject("exportData");
    layer->exportData_.resolutionScale = 
        exportData.read<float>("resolutionScale");
    layer->exportData_.rescaleWithOutput = 
        exportData.read<bool>("rescaleWithOutput");
    layer->exportData_.windowResolutionScale = 
        exportData.read<float>("windowResolutionScale");
    layer->exportData_.resolution =
        (glm::vec2)layer->resolution_ * 
        layer->exportData_.resolutionScale *
        layer->exportData_.windowResolutionScale +.5f;

    layer->flags_.isAspectRatioBoundToWindow = 
        io.read<bool>("isAspectRatioBoundToWindow");

    auto shaderData = io.readObject("shader");
    auto fragmentSource = shaderData.read("fragmentSource", false);

    Uniform::loadAll
    (
        shaderData,
        layer->uniforms_,
        resources,
        layer->cache_.uninitializedResourceLayers
    );
    
    layer->gui_.sourceEditor.setText(fragmentSource);
    layer->gui_.sourceEditor.resetTextChanged();

    // If the project was saved in a state such that the shader has compilation
    // errors, then initialize the shader with the blank shader source (back-end
    // -only, the user will still see the source of the saved shader with the 
    // usual list of compilation errors and markers)
    if (!layer->compileShader(sharedUniforms))
    {
        layer->rendering_.shader = 
            vir::Shader::create
                (
                    vertexShaderSource(sharedUniforms),
                    glslVersionSource()+
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})",
                    vir::Shader::ConstructFrom::SourceCode
                );
    }

    auto framebufferData = io.readObject("internalFramebuffer");
    auto internalFormat = 
        (vir::TextureBuffer::InternalFormat)framebufferData.read<int>("format");
    
    layer->rebuildFramebuffers(internalFormat, layer->resolution_);

    // Set framebuffer color attachment wrapping and filtering settings
    auto magFilter = framebufferData.read<int>("magFilterMode");
    auto minFilter = framebufferData.read<int>("minFilterMode");
    auto wrapModes = framebufferData.read<glm::ivec2>("wrapModes");
    layer->setFramebufferMagFilterMode((FilterMode)magFilter);
    layer->setFramebufferMinFilterMode((FilterMode)minFilter);
    layer->setFramebufferWrapMode(0, (WrapMode)wrapModes[0]);
    layer->setFramebufferWrapMode(1, (WrapMode)wrapModes[1]);

    layer->exportData_.clearPolicy = 
        (ExportData::FramebufferClearPolicy)
        framebufferData.read<int>("exportClearPolicy");

    // Initialize post-processing effects, if any were saved 
    if (io.hasMember("postProcesses"))
    {
        auto postProcessData = io.readObject("postProcesses");
        for (auto name : postProcessData.members())
        {
            ObjectIO data(postProcessData.readObject(name));
            layer->rendering_.postProcesses.emplace_back
            (
                PostProcess::load(data, layer)
            );
        }
    }

    //
    if (layer->rendering_.target != Rendering::Target::Window)
        LayerResource::insertInResources
        (
            layer,
            resources
        );
    return layer;
}

//----------------------------------------------------------------------------//

void Layer::loadAll
(
    const ObjectIO& io,
    std::vector<Layer*>& layers, 
    SharedUniforms& sharedUniforms,
    std::vector<Resource*>& resources
)
{
    // Clear state
    for (auto layer : layers)
        delete layer;
    layers.clear();
    
    if (io.hasMember("sharedFragmentSource"))
    {
        Layer::GUI::sharedSourceEditor.setText
        (
            io.read("sharedFragmentSource", false)
        );
        Layer::GUI::sharedSourceEditor.resetTextChanged();
    }
    else
        Layer::resetSharedSourceEditor();

    Layer::Rendering::sharedStorage = 
        std::unique_ptr<SharedStorage>(SharedStorage::load(io));
    
    auto ioLayers = io.readObject("layers");
    for (auto ioLayerName : ioLayers.members())
    {   
        auto ioLayer = ioLayers.readObject(ioLayerName);
        auto layer = Layer::load(ioLayer, layers, sharedUniforms, resources);
        layers.emplace_back(layer);
    };

    // Re-establish dependencies between Layers (i.e., when a layer is
    // being used as a sampler2D uniform by another layer)
    for (auto* layer : layers)
    {
        for (auto& entry : layer->cache_.uninitializedResourceLayers)
        {
            auto* uniform = entry.first;
            auto& layerName = entry.second;
            for (auto resource : resources)
            {
                if (resource->name() != layerName)
                    continue;
                uniform->setValuePtr<Resource>(resource);
            }
        }
        layer->cache_.uninitializedResourceLayers.clear();
    }

    // Same exact thing as what has been done for the layer uniforms, but here
    // done for shared user-custom uniforms
    sharedUniforms.postLoadProcessCachedResourceLayers(resources);
}

//----------------------------------------------------------------------------//

void Layer::onReceive(vir::Event::WindowResizeEvent& event)
{
    glm::ivec2 resolution = {event.width, event.height};
    setResolution(resolution, true);
}

//----------------------------------------------------------------------------//

unsigned int Layer::findFreeId(const std::vector<Layer*>& layers)
{
    std::vector<unsigned int> ids(layers.size());
    unsigned int id(0);
    for (auto l : layers)
        ids[id++] = l->id_;
    std::sort(ids.begin(), ids.end());
    for (id=0; id<layers.size(); id++)
    {
        if (id < ids[id])
            return id;
    }
    return id;
}

//----------------------------------------------------------------------------//

const std::string& Layer::glslVersionSource()
{
    static const auto window = vir::Window::instance();
    static const std::string version
    (
        "#version "+window->context()->shadingLanguageVersion()+" core\n"
    );
    return version;
}

//----------------------------------------------------------------------------//

const std::string& Layer::vertexShaderSource
(
    const SharedUniforms& sharedUniforms
)
{
    static const std::string vertexSource
    (
        glslVersionSource()+
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
)" + sharedUniforms.glslVertexBlockSource() +
R"(
void main(){
    gl_Position = iMVP*vec4(iqc, 1.);
    qc = iqc.xy;
    tc = itc;})"
    );
    return vertexSource;
}

//----------------------------------------------------------------------------//

std::tuple<std::string, unsigned int> 
Layer::fragmentShaderHeaderSourceAndLineCount
(
    const SharedUniforms& sharedUniforms
) const
{
    std::string header
    (
        glslVersionSource()+
        "in      vec2   qc;\nin      vec2   tc;\nout     vec4   fragColor;\n" +
        Rendering::sharedStorage->glslBlockSource() +
        sharedUniforms.glslFragmentBlockSource() +
        "\n"
    );
    auto nLines = Helpers::countNewLines(header);

    for (auto* u : sharedUniforms.userUniforms())
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
        header += 
            "uniform "+typeName+" "+u->name+";\n";
        ++nLines;
        // Automatically managed sampler2D resolution and aspect-ratio uniforms
        if (isSampler2D)
        {
            header += "uniform float "+u->name+"AspectRatio;\n";
            ++nLines;
            header += "uniform vec2 "+u->name+"Resolution;\n";
            ++nLines;
        }
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
        header += 
            "uniform "+typeName+" "+u->name+";\n";
        ++nLines;
        // Automatically managed sampler2D resolution and aspect-ratio uniforms
        if (isSampler2D)
        {
            header += "uniform float "+u->name+"AspectRatio;\n";
            ++nLines;
            header += "uniform vec2 "+u->name+"Resolution;\n";
            ++nLines;
        }
    }

    return {header, nLines};
}

//----------------------------------------------------------------------------//

void Layer::setResolution
(
    const glm::ivec2& iResolution,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio,
    const bool setExportResolution
)
{
    glm::ivec2 resolution = iResolution;
    static const auto* window(vir::Window::instance());
    glm::vec2 windowResolution(window->width(), window->height());
    if (windowFrameManuallyDragged)
    {
        resolution = 
            glm::max(resolutionRatio_*(glm::vec2)resolution+.5f, {1,1});
        auto viewport = Helpers::normalizedWindowResolution();
        rendering_.quad->update(viewport.x, viewport.y, depth_);
        if (!flags_.rescaleWithWindow)
            return;
    }
    else if (!window->iconified())
        resolutionRatio_ = (glm::vec2)resolution/windowResolution;
    
    if (resolution == resolution_)
        return;
    
    if 
    (
        tryEnfoceWindowAspectRatio && 
        flags_.isAspectRatioBoundToWindow &&
        !window->iconified()
    )
    {
        float windowAspectRatio = window->aspectRatio();
        if (resolution.x == resolution_.x)
        {
            resolution_.x = resolution.y*windowAspectRatio+.5f;
            resolution_.y = resolution.y;
        }
        else if (resolution.y == resolution_.y)
        {
            resolution_.y = resolution.x/windowAspectRatio+.5f;
            resolution_.x = resolution.x;
        }
        resolutionRatio_ = (glm::vec2)resolution_/windowResolution;        
    }
    else
        resolution_ = resolution;
    aspectRatio_ = ((float)resolution_.x)/resolution_.y;
    
    if (setExportResolution)
        exportData_.resolution = 
            (glm::vec2)resolution_*
            exportData_.resolutionScale*
            exportData_.windowResolutionScale + .5f;
    
    rebuildFramebuffers
    (
        rendering_.framebuffer == nullptr ?
        vir::TextureBuffer::InternalFormat::RGBA_SF_32 :
        rendering_.framebuffer->colorBufferInternalFormat(),
        resolution_
    );
    if (rendering_.shader == nullptr)
        return;
    rendering_.shader->bind();
    rendering_.shader->setUniformFloat("iAspectRatio", aspectRatio_);
    rendering_.shader->setUniformFloat2("iResolution", (glm::vec2)resolution_);
}

//----------------------------------------------------------------------------//

void Layer::setName(const std::string& name)
{
    gui_.name = name;
    gui_.newName = gui_.name;
    flags_.rename = false;
}

//----------------------------------------------------------------------------//

void Layer::setDepth(const float depth)
{
    depth_ = depth;
    if (rendering_.quad != nullptr)
        rendering_.quad->update
        (
            rendering_.quad->width(),
            rendering_.quad->height(),
            depth_
        );
    else
    {
        auto viewport = Helpers::normalizedWindowResolution();
        rendering_.quad = new vir::Quad(viewport.x, viewport.y, depth_);
    }
}

//----------------------------------------------------------------------------//

void Layer::setFramebufferWrapMode(int i, WrapMode mode)
{
    rendering_.framebufferA->setColorBufferWrapMode(i, mode);
    rendering_.framebufferB->setColorBufferWrapMode(i, mode);
}

//----------------------------------------------------------------------------//

void Layer::setFramebufferMagFilterMode(FilterMode mode)
{
    rendering_.framebufferA->setColorBufferMagFilterMode(mode);
    rendering_.framebufferB->setColorBufferMagFilterMode(mode);
}

//----------------------------------------------------------------------------//

void Layer::setFramebufferMinFilterMode(FilterMode mode)
{
    rendering_.framebufferA->setColorBufferMinFilterMode(mode);
    rendering_.framebufferB->setColorBufferMinFilterMode(mode);
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
        vir::GeometricPrimitive& quad,
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    )
    {
        if (framebuffer != nullptr)
        {
            auto wrapModeX = framebuffer->colorBufferWrapMode(0);
            auto wrapModeY = framebuffer->colorBufferWrapMode(1);
            auto minFilterMode = framebuffer->colorBufferMinFilterMode();
            auto magFilterMode = framebuffer->colorBufferMagFilterMode();
            
            // Preserve original framebuffer contents after resizing
            auto newFramebuffer = vir::Framebuffer::create
            (
                resolution.x,
                resolution.y,
                internalFormat
            );
            Layer::Rendering::textureMapperShader->bind();
            Layer::Rendering::textureMapperShader->setUniformInt("tx", 0);
            framebuffer->bindColorBuffer(0);
            
            // This rendering step is to copy the original framebuffer contents
            // to the new framebuffer according to the original framebuffer
            // filtering options
            vir::Renderer::instance()->submit
            (
                quad, 
                Layer::Rendering::textureMapperShader.get(), 
                newFramebuffer
            );
            framebuffer->unbind();
            DELETE_IF_NOT_NULLPTR(framebuffer)
            framebuffer = newFramebuffer;

            framebuffer->setColorBufferWrapMode(0, wrapModeX);
            framebuffer->setColorBufferWrapMode(1, wrapModeY);
            framebuffer->setColorBufferMinFilterMode(minFilterMode);
            framebuffer->setColorBufferMagFilterMode(magFilterMode);
        }
        else
            framebuffer = vir::Framebuffer::create
            (
                resolution.x, 
                resolution.y, 
                internalFormat
            );
    };
    rebuildFramebuffer
    (
        rendering_.framebufferA, 
        *rendering_.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    rebuildFramebuffer
    (
        rendering_.framebufferB, 
        *rendering_.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    rendering_.framebuffer = rendering_.framebufferA;
}

//----------------------------------------------------------------------------//

void Layer::clearFramebuffers()
{
    rendering_.framebufferA->clearColorBuffer();
    rendering_.framebufferB->clearColorBuffer();
}

//----------------------------------------------------------------------------//

bool Layer::compileShader(const SharedUniforms& sharedUniforms)
{
    auto headerAndLineCount = 
        fragmentShaderHeaderSourceAndLineCount(sharedUniforms);
    gui_.sourceHeader = std::get<std::string>(headerAndLineCount);
    unsigned int nHeaderLines = std::get<unsigned int>(headerAndLineCount);
    unsigned int nSharedLines = GUI::sharedSourceEditor.getTotalLines()+1;
    auto shader = vir::Shader::create
    (
        vertexShaderSource(sharedUniforms),
        (
            gui_.sourceHeader +
            gui_.sharedSourceEditor.getText()+"\n"+
            gui_.sourceEditor.getText()
        ),
        vir::Shader::ConstructFrom::SourceCode
    );
    if (shader->valid())
    {
        delete rendering_.shader;
        rendering_.shader = shader;
        gui_.headerErrors.clear();
        gui_.sourceEditor.setErrorMarkers({});
        gui_.sharedSourceEditor.setErrorMarkers({});
        cache_.uncompiledUniforms.erase
        (
            std::remove_if
            (
                cache_.uncompiledUniforms.begin(),
                cache_.uncompiledUniforms.end(),
                [](Uniform* u){return u->name.size()>0;}
            ),
            cache_.uncompiledUniforms.end()
        );
        flags_.uncompiledChanges = false;
        // Re-set uniforms
        sharedUniforms.bindShader(rendering_.shader);
        Rendering::sharedStorage->bindShader(rendering_.shader);
        rendering_.shader->bind();
        #define CASE(ST, T, F)                                              \
        case Uniform::Type::ST :                                            \
        {                                                                   \
            T value = u->getValue<T>();                                     \
            u->setValue(value);                                             \
            if (named)                                                      \
                rendering_.shader->F(u->name, value);                       \
            break;                                                          \
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
        for (auto u : sharedUniforms.userUniforms())
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
        rendering_.shader->setUniformFloat("iAspectRatio", aspectRatio_);
        rendering_.shader->setUniformFloat2
        (
            "iResolution", 
            (glm::vec2)resolution_
        );
        return true;
    }
    // Else if shader not valid
    std::map<int, std::string> sourceErrors, sharedErrors;
    for (const auto& error : shader->compilationErrors().fragmentErrors)
    {
        int sourceLineNo(error.first - nSharedLines - nHeaderLines + 1);
        int sharedLineNo(error.first - nHeaderLines + 1);
        if (sourceLineNo > 0)
            sourceErrors.insert({sourceLineNo, error.second});
        else if (sharedLineNo > 0)
            sharedErrors.insert({sharedLineNo, error.second});
        else 
        {
            if (gui_.headerErrors.size() > 0)
                gui_.headerErrors += "\n";
            gui_.headerErrors += "Header: " + error.second;
        }
    }
    auto setEditorErrors = []
    (
        TextEditor& editor,
        const std::map<int, std::string>& errors
    )
    {
        editor.setErrorMarkers(errors);
        if (errors.size() > 0)
            editor.setCursorPosition({errors.begin()->first, 0});
    };
    setEditorErrors(gui_.sourceEditor, sourceErrors);
    setEditorErrors(gui_.sharedSourceEditor, sharedErrors);
    delete shader;
    return false;
}

//----------------------------------------------------------------------------//

void Layer::renderShader
(
    vir::Framebuffer* target,
    const bool clearTarget,
    const SharedUniforms& sharedUniforms
)
{
    // Set sampler-type uniforms found in both this layer's uniforms as well
    // as the shared user-added uniforms
    rendering_.shader->bind();
    unsigned int unit = 0; 

    auto setSamplerUniforms = []
    (
        const std::vector<Uniform*>& uniforms,
        Layer* layer, 
        unsigned int& unit
    )
    {
        vir::Shader* shader(layer->rendering_.shader);
        for (auto u : uniforms)
        {
            if 
            (

                u->specialType != Uniform::SpecialType::None ||
                u->name.size() == 0 || 
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

            // These two lines are required when a layer reads from its own 
            // framebuffer to avoid visual artifacts. It makes it so that the
            // target read framebuffer is the framebuffer of the previous
            // render step
            if (resource->name() == layer->gui_.name)
            {
                // Buffers are flipped afterwards, so rendering_.framebuffer
                // is guaranteed to contain the read-only framebuffer pointer
                vir::Framebuffer* sourceFramebuffer = 
                    layer->rendering_.framebuffer;
                for (auto* postProcess : layer->rendering_.postProcesses)
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
            shader->setUniformInt(u->name, unit);
            unit++;
            // Set the (automatically managed) sampler2D resolution uniform 
            // value. Should find a better way rather than setting this every
            // render call
            if (u->type == vir::Shader::Variable::Type::Sampler2D)
            {
                shader->setUniformFloat
                (
                    u->name+"AspectRatio", 
                    float(resource->width())/resource->height()
                );
                shader->setUniformFloat2
                (
                    u->name+"Resolution", 
                    {resource->width(), resource->height()}
                );
            }
        }
    };
    setSamplerUniforms(sharedUniforms.userUniforms(), this, unit);
    setSamplerUniforms(uniforms_, this, unit);

    // Flip buffers
    rendering_.framebuffer = 
        rendering_.framebuffer == rendering_.framebufferB ? 
        rendering_.framebufferA :
        rendering_.framebufferB;

    // Set all uniforms that are not set in the GUI step
    // setSharedDefaultSamplerUniforms();
    
    // Re-direct rendering & disable blending if not rendering to the window
    static auto renderer = vir::Renderer::instance();
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (rendering_.target != Layer::Rendering::Target::Window)
    {
        target = rendering_.framebuffer;
        renderer->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    renderer->submit
    (
        *rendering_.quad,
        rendering_.shader,
        target,
        clearTarget || // Or force clear if not rendering to window
        rendering_.target != Layer::Rendering::Target::Window
    );
    Rendering::sharedStorage->gpuMemoryBarrier();

    // Re-enable blending before either leaving or redirecting the rendered 
    // texture to the main window
    if (!blendingEnabled)
        renderer->setBlending(true);

    // Apply post-processing effects, if any
    for (auto postProcess : rendering_.postProcesses)
        postProcess->run();

    if 
    (
        rendering_.target != 
        Layer::Rendering::Target::InternalFramebufferAndWindow
    )
        return;

    // Render texture rendered by the previous call to the provided initial 
    // target (or to main window if target0 == nullptr)
    Layer::Rendering::textureMapperShader->bind();
    rendering_.framebuffer->bindColorBuffer(0);
    Layer::Rendering::textureMapperShader->setUniformInt("tx", 0);
    renderer->submit
    (
        *rendering_.quad, 
        Layer::Rendering::textureMapperShader.get(), 
        target0,
        clearTarget
    );
}

//----------------------------------------------------------------------------//

void Layer::prepareForExport()
{
    exportData_.originalResolution = resolution_;
    setResolution(exportData_.resolution, false, false, false);
}

//----------------------------------------------------------------------------//

void Layer::resetAfterExport()
{
    setResolution(exportData_.originalResolution, false, false, false);
}

//----------------------------------------------------------------------------//

bool Layer::removeResourceFromUniforms(const Resource* resource)
{
    for (int i=0; i<(int)uniforms_.size(); i++)
    {
        auto uniform = uniforms_[i];
        if 
        (
            uniform->type != Uniform::Type::Sampler2D &&
            uniform->type != Uniform::Type::SamplerCube
        )
            continue;
        auto uResource = uniform->getValuePtr<const Resource>();
        if (resource->id() == uResource->id())
        {
            uniforms_.erase(uniforms_.begin()+i);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------//

unsigned int Layer::renderShaders // Static
(
    const std::vector<Layer*>& layers,
    vir::Framebuffer* target, 
    SharedUniforms& sharedUniforms,
    const unsigned int nRenderPasses,
    const bool renderNextFrame
)
{
    bool clearTarget = true;
    unsigned int iRenderPass = sharedUniforms.iRenderPass();
    if (renderNextFrame)
    {
        if (target != nullptr && iRenderPass == 0) // I.e., if exporting
        {
            for (auto layer : layers) // Apply clear policy
            {
                switch (layer->exportData_.clearPolicy)
                {
                case ExportData::FramebufferClearPolicy::None :
                    continue;
                case ExportData::FramebufferClearPolicy::ClearOnFirstFrameExport:
                    if (sharedUniforms.iFrame() == 0)
                        layer->clearFramebuffers();
                    break;
                case ExportData::FramebufferClearPolicy::ClearOnEveryFrameExport:
                    if (iRenderPass == 0)
                        layer->clearFramebuffers();
                    break;
                }
            }
        }

        clearTarget = true;
        for (auto layer : layers)
        {
            layer->renderShader(target, clearTarget, sharedUniforms);
            // At the end of this loop, the status of clearRenderingTarget will 
            // represent whether the main window has been cleared of its 
            // contents at least once (true if never cleared at least once)
            if 
            (
                clearTarget &&
                layer->rendering_.target != 
                    Layer::Rendering::Target::InternalFramebuffer
            )
                clearTarget = false;
        }
        // Advance iRenderPass uniform, reset to 0 if it exceeds nRenderPasses-1
        sharedUniforms.nextRenderPass(nRenderPasses);
    }

    // If the window has not been cleared at least once, or if I am not
    // rendering to the window at all (i.e., if renderTarget != nullptr, which is only
    // true during exports), then render a dummy/void/blank window, simply to
    // avoid visual artifacts when nothing is rendering to the main window.
    // As for the internalFramebufferShader in Layer::renderShader, the lifetime
    // of the shader (and quad) is managed statically within here
    if (clearTarget || target != nullptr)
    {
        static std::unique_ptr<vir::Quad> blankQuad(new vir::Quad(1, 1, 0));
        auto viewport = Helpers::normalizedWindowResolution();
        blankQuad->update(viewport.x, viewport.y, 0);
        auto constructBlankShader = [&sharedUniforms]()
        {
            auto shader = std::unique_ptr<vir::Shader>
            (
                vir::Shader::create
                (
                    vertexShaderSource(sharedUniforms),
                    glslVersionSource()+
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})",
                    vir::Shader::ConstructFrom::SourceCode
                )
            );
            sharedUniforms.bindShader(shader.get());
            return shader;
        };
        static auto blankShader = constructBlankShader();
        vir::Renderer::instance()->submit
        (
            *blankQuad.get(), 
            blankShader.get()
        );
    }

    return iRenderPass;
}

//----------------------------------------------------------------------------//

void Layer::renderFramebufferPropertiesGui()
{
    const float entryWidth(14*ImGui::GetFontSize());
    ImGui::Text("Internal data format ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerInternalFormatCombo",
            vir::TextureBuffer::internalFormatToName.at
            (
                rendering_.framebuffer->
                    colorBufferInternalFormat()
            ).c_str()
        )
    )
    {
        static vir::TextureBuffer::InternalFormat 
        supportedInternalFormats[2]
        {
            vir::TextureBuffer::InternalFormat::RGBA_UNI_8, 
            vir::TextureBuffer::InternalFormat::RGBA_SF_32
        };
        for (auto internalFormat : supportedInternalFormats)
        {
            if 
            (
                ImGui::Selectable
                (
                    vir::TextureBuffer::internalFormatToName.at
                    (
                        internalFormat
                    ).c_str()
                )
            )
                rebuildFramebuffers
                (
                    internalFormat,
                    resolution_
                );
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    //
    std::string selectedWrapModeX = "";
    std::string selectedWrapModeY = "";
    std::string selectedMagFilterMode = "";
    std::string selectedMinFilterMode = "";
    if (rendering_.framebuffer != nullptr)
    {
        selectedWrapModeX = vir::TextureBuffer::wrapModeToName.at
        (
            rendering_.framebuffer->colorBufferWrapMode(0)
        );
        selectedWrapModeY = vir::TextureBuffer::wrapModeToName.at
        (
            rendering_.framebuffer->colorBufferWrapMode(1)
        );
        selectedMagFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                rendering_.framebuffer->colorBufferMagFilterMode()
            );
        selectedMinFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                rendering_.framebuffer->colorBufferMinFilterMode()
            );
    }
    ImGui::Text("Horizontal wrap mode ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerWrapModeXCombo",
            selectedWrapModeX.c_str()
        ) && rendering_.framebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::wrapModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferWrapMode(0, entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
    ImGui::Text("Vertical   wrap mode ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerWrapModeYCombo",
            selectedWrapModeY.c_str()
        ) && rendering_.framebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::wrapModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferWrapMode(1, entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Magnification filter ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerMagModeCombo",
            selectedMagFilterMode.c_str()
        ) && rendering_.framebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::filterModeToName)
        {
            if 
            (
                entry.first != FilterMode::Nearest&&
                entry.first != FilterMode::Linear
            )
                continue;
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferMagFilterMode(entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    ImGui::Text("Minimization  filter ");
    ImGui::SameLine();
    ImGui::PushItemWidth(entryWidth);
    if 
    (
        ImGui::BeginCombo
        (
            "##layerMinModeCombo",
            selectedMinFilterMode.c_str()
        ) && rendering_.framebuffer != nullptr
    )
    {
        for (auto entry : vir::TextureBuffer::filterModeToName)
        {
            if (ImGui::Selectable(entry.second.c_str()))
                setFramebufferMinFilterMode(entry.first);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();
}

//----------------------------------------------------------------------------//

void Layer::renderPropertiesMenuGui(std::vector<Resource*>& resources)
{
    if (ImGui::BeginMenu(("Layer ["+gui_.name+"]").c_str()))
    {
        const float fontSize(ImGui::GetFontSize());
        const float entryWidth(14*fontSize);
        ImGui::Text("Name                 ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", id_);
        ImGui::PushItemWidth(entryWidth);
        if (ImGui::InputText(label.get(), &gui_.newName))
            flags_.rename = true; // The actual renaming happens in the static
                                  // function Layer::renderLayersTabBarGui(...)
        ImGui::PopItemWidth();

        static std::map<Rendering::Target, const char*> 
        renderTargetToName
        {
            {Rendering::Target::InternalFramebufferAndWindow, "Framebuffer & window"},
            {Rendering::Target::InternalFramebuffer, "Framebuffer"},
            {Rendering::Target::Window, "Window"}
        };
        ImGui::Text("Render target        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::BeginCombo
            (
                "##rendererTarget", 
                renderTargetToName.at(rendering_.target)
            )
        )
        {
            for(auto entry : renderTargetToName)
            {
                if (!ImGui::Selectable(entry.second))
                    continue;
                auto target = entry.first;
                if (target != rendering_.target)
                {
                    rendering_.target = target;
                    if (rendering_.target == Rendering::Target::Window)
                        LayerResource::removeFromResources
                        (
                            this,
                            resources
                        );
                    else
                        LayerResource::insertInResources
                        (
                            this,
                            resources
                        );
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if 
        (
            rendering_.target == Rendering::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::BeginDisabled();
        ImGui::Text("Resolution           ");
        ImGui::SameLine();
        auto x0 = ImGui::GetCursorPos().x;
        if 
        (
            ImGui::Button
            (
                flags_.isAspectRatioBoundToWindow ? 
                " " ICON_FA_LOCK " " : 
                " " ICON_FA_LOCK_OPEN " "
            )
        )
        {
            flags_.isAspectRatioBoundToWindow = 
                !flags_.isAspectRatioBoundToWindow;
            if (flags_.isAspectRatioBoundToWindow)
            {
                auto window = vir::Window::instance();
                glm::ivec2 resolution = {window->width(), window->height()};
                setResolution(resolution, false);
            }
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
                flags_.isAspectRatioBoundToWindow ?
ICON_FA_LOCK " - The aspect ratio is locked\n"
"to that of the main window" :
ICON_FA_LOCK_OPEN " - The aspect ratio is not locked\n"
"to that of the main window"
            );
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        auto aspectRatioLockSize = ImGui::GetCursorPos().x-x0;
        ImGui::PushItemWidth(entryWidth-aspectRatioLockSize);
        glm::ivec2 resolution = resolution_;
        std::sprintf(label.get(), "##layer%dResolution", id_);
        if (ImGui::InputInt2(label.get(), glm::value_ptr(resolution)))
            setResolution(resolution, false, true);
        ImGui::PopItemWidth();
        ImGui::Text("Auto-resize mode     ");
        ImGui::SameLine();
        if
        (
            ImGui::Button
            (
                flags_.rescaleWithWindow ?
                "Rescale on window resize" :
                "Do not auto-resize",
                {-1, 0}
            )
        )
            flags_.rescaleWithWindow = !flags_.rescaleWithWindow;
        if 
        (
            rendering_.target == Rendering::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::EndDisabled();
    
        if (rendering_.target != Rendering::Target::Window)
        {
            ImGui::SeparatorText("Framebuffer settings");
            renderFramebufferPropertiesGui();

            ImGui::SeparatorText("Post-processing effects");
            int iDelete = -1;
            static int iActive = -1;
            auto& postProcesses = rendering_.postProcesses;
            for (int i = 0; i < (int)postProcesses.size(); i++)
            {
                PostProcess* postProcess = postProcesses[i];
                ImGui::PushID(i);
                if (ImGui::SmallButton(ICON_FA_TRASH))
                    iDelete = i;
                ImGui::SameLine();
                if 
                (
                    ImGui::BeginMenu
                    (
                        std::string
                        (
                            std::to_string(i+1)+" - "+postProcess->name()
                        ).c_str()
                    )
                )
                {
                    // This part here is for the click & drag re-order mechanics
                    if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
                    {
                        if (iActive == -1)
                            iActive = i;
                        else if (iActive != i)
                        {
                            postProcesses[i] = postProcesses[iActive];
                            postProcesses[iActive] = postProcess;
                            iActive = -1;
                        }
                    }
                    else
                        iActive = -1;
                    // Render post-processing effect GUI
                    if (!postProcess->canRunOnDeviceInUse())
                    {
                        ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                        ImGui::Text(postProcess->errorMessage().c_str());
                        ImGui::PopTextWrapPos();
                    }
                    else
                        postProcess->renderGui();
                    ImGui::EndMenu();
                }
                ImGui::PopID();
            }
            if (iDelete != -1)
            {
                auto* postProcess = postProcesses[iDelete];
                delete postProcess;
                postProcess = nullptr;
                postProcesses.erase(postProcesses.begin()+iDelete);
            }
            
            // Selector for adding a new post-processing effect with the 
            // constraint that each layer may have at most one 
            // tpost-processing effect of each ype
            ImGui::PushItemWidth(-1);
            if 
            (
                ImGui::BeginCombo
                (
                    "##postProcessingCombo", 
                    "Add a post-processing effect"
                )
            )
            {
                static std::vector<vir::PostProcess::Type> allAvailableTypes(0);
                if (allAvailableTypes.size() == 0)
                {
                    allAvailableTypes.reserve
                    (
                        vir::PostProcess::typeToName.size()
                    );
                    for (auto kv : vir::PostProcess::typeToName)
                    {
                        if (kv.first != vir::PostProcess::Type::Undefined)
                            allAvailableTypes.push_back(kv.first);
                    }
                }
                std::vector<vir::PostProcess::Type> 
                    availableTypes(allAvailableTypes);
                for (auto* postProcess : postProcesses)
                {
                    auto it = std::find
                    (
                        availableTypes.begin(), 
                        availableTypes.end(), 
                        postProcess->type()
                    );
                    if (it != availableTypes.end())
                        availableTypes.erase(it);
                }
                for (auto type : availableTypes)
                {
                    if 
                    (
                        ImGui::Selectable
                        (
                            vir::PostProcess::typeToName.at(type).c_str()
                        )
                    )
                    {
                        PostProcess* postProcess = 
                            PostProcess::create(this, type);
                        if (postProcess != nullptr)
                            postProcesses.emplace_back(postProcess);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            
        }
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderLayersTabBarGui // Static
(
    std::vector<Layer*>& layers,
    SharedUniforms& sharedUnifoms,
    std::vector<Resource*>& resources
)
{
    static bool compilationErrors(false);
    static bool uncompiledEdits(false);
    if (uncompiledEdits || compilationErrors) // Render compilation button ------
    {
        float time = vir::Time::instance()->outerTime();
        ImVec4 compileButtonColor = 
        {
            .5f*glm::sin(6.283f*(time/3+0.f/3))+.3f,
            .5f*glm::sin(6.283f*(time/3+1.f/3))+.3f,
            .5f*glm::sin(6.283f*(time/3+2.f/3))+.3f,
            1.f
        };
        ImGui::PushStyleColor(ImGuiCol_Button, compileButtonColor);
        if 
        (
            ImGui::ArrowButton("##right",ImGuiDir_Right) ||
            Helpers::isCtrlKeyPressed(ImGuiKey_B)
        )
        {
            ImGui::SetTooltip("Compiling project shaders...");
            for (auto layer : layers)
                layer->compileShader(sharedUnifoms);
        }
        ImGui::PopStyleColor();
        ImGui::SameLine();
        ImGui::Text("Compile all shaders");
        ImGui::SameLine();
        static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
        ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
        ImGui::Text("Ctrl+B");
        ImGui::PopStyleColor();
    }

    // Render list of compilation errors with formatting -----------------------
    bool errorColorPushed = false;
    if (compilationErrors)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, {1,0,0,1});
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    compilationErrors = false;
    uncompiledEdits = false;
    const auto& sharedErrors // First render errors in shared source -----------
    (
        Layer::GUI::sharedSourceEditor.getErrorMarkers()
    );
    if (sharedErrors.size() > 0)
    {
        compilationErrors = true;
        ImGui::Bullet(); ImGui::Text("Common");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            for (auto& error : sharedErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
    for (auto* layer : layers) // Second, render layer-specific errors in either
                               // source header or editable source -------------
    {
        const auto& sourceErrors(layer->gui_.sourceEditor.getErrorMarkers());
        if (sourceErrors.size() > 0 || layer->gui_.headerErrors.size() > 0)
        {
            compilationErrors = true;
            ImGui::Bullet();ImGui::Text(layer->gui_.name.c_str());
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                if (layer->gui_.headerErrors.size() > 0)
                    ImGui::Text(layer->gui_.headerErrors.c_str());
                for (auto& error : sourceErrors)
                {
                    // First is line no., second is actual error text
                    std::string errorText = 
                        "Line "+std::to_string(error.first)+": "+error.second;
                    ImGui::Text(errorText.c_str());
                }
                ImGui::EndTooltip();
            }
        }
        if
        (
            layer->gui_.sourceEditor.isTextChanged() || 
            Layer::GUI::sharedSourceEditor.isTextChanged()
        )
            layer->flags_.uncompiledChanges = true;
        if 
        (
            Layer::GUI::sharedSourceEditor.isTextChanged() ||
            layer->flags_.uncompiledChanges
        )
            uncompiledEdits = true;
    }
    if (compilationErrors)
        ImGui::Separator();
    if (errorColorPushed)
        ImGui::PopStyleColor();

    // Actual layers tab bar ---------------------------------------------------
    static bool reorderable(true);
    if 
    (
        ImGui::BeginTabBar
        (
            "##layersTabBar", 
            reorderable ? 
            ImGuiTabBarFlags_AutoSelectNewTabs | ImGuiTabBarFlags_Reorderable :
            ImGuiTabBarFlags_AutoSelectNewTabs
        )
    )
    {
        reorderable = true;
        if (ImGui::TabItemButton("+", ImGuiTabItemFlags_Trailing))
            layers.emplace_back(new Layer(layers, sharedUnifoms));
        auto tabBar = ImGui::GetCurrentTabBar();
        std::pair<unsigned int, unsigned int> swap {0,0};
        for (int i = 0; i < (int)layers.size(); i++)
        {
            bool open = true;
            auto layer = layers[i];
            if
            (
                ImGui::BeginTabItem
                (
                    layer->gui_.name.c_str(), 
                    &open, 
                    layers.size() == 1 ? ImGuiTabItemFlags_NoCloseButton : 0
                )
            )
            {
                layer->renderTabBarGui(layers, sharedUnifoms, resources);
                ImGui::EndTabItem();
            }
            bool deleted(false);
            if (!open) // I.e., if 'x' is pressed to delete the tab
            {
                ImGui::OpenPopup("Layer deletion confirmation");
                layer->flags_.pendingDeletion = true;
                reorderable = false;
            }
            if 
            (
                layer->flags_.pendingDeletion && 
                ImGui::BeginPopupModal
                (
                    "Layer deletion confirmation",
                    0,
                    ImGuiWindowFlags_NoResize
                )
            )
            {
                ImGui::Text
                (
                    "Are you sure you want to delete '%s'?", 
                    layer->gui_.name.c_str()
                );
                ImGui::Text("This action cannot be undone!");
                if (ImGui::Button("Delete"))
                {
                    layers.erase
                    (
                        std::remove_if
                        (
                            layers.begin(), 
                            layers.end(), 
                            [&layer](const Layer* l) {return l==layer;}
                        ), 
                        layers.end()
                    );
                    LayerResource::removeFromResources
                    (
                        layer,
                        resources
                    );
                    delete layer;
                    --i;
                    deleted = true;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::SameLine();
                if (ImGui::Button("Cancel"))
                {
                    layer->flags_.pendingDeletion = false;
                    ImGui::CloseCurrentPopup();
                }
                ImGui::EndPopup();
            }
            if (deleted)
                continue;
            auto tab = ImGui::TabBarFindTabByID
            (
                tabBar, 
                ImGui::GetID(layer->gui_.name.c_str())
            );
            if // the name has changed, make the tab bar non-reorderable
            (  // and update the name
                tab == nullptr || 
                (layer->flags_.rename && !ImGui::GetIO().WantTextInput)
            )
            {
                reorderable = false;
                layer->setName(layer->gui_.newName);
                continue;
            }
            int order = std::min
            (
                ImGui::TabBarGetTabOrder(tabBar, tab),
                (int)layers.size()-1
            );
            if (order != i)
                swap = {order, i};
        }
        if (swap.first != swap.second)
        {
            auto l1 = layers[swap.first];
            auto l2 = layers[swap.second];
            auto tl = l1;
            auto td = l1->depth_;
            layers[swap.first] = l2;
            layers[swap.second] = tl;
            l1->setDepth(l2->depth_);
            l2->setDepth(td);
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }

    // Check if layers framebuffers should be cleared as a consequence of
    // a rendering restart. This flag is set in the lambda
    // Uniform::renderUniformsGui::renderSharedUniformsGui eventually called by
    // renderTabBarGui
    if (Layer::Flags::restartRendering)
    {
        for (auto layer : layers)
        {
            layer->rendering_.framebufferA->clearColorBuffer();
            layer->rendering_.framebufferB->clearColorBuffer();
        }
        Layer::Flags::restartRendering = false;
    }
}

//----------------------------------------------------------------------------//

void Layer::renderTabBarGui
(
    const std::vector<Layer*>& layers,
    SharedUniforms& sharedUnifoms,
    std::vector<Resource*>& resources
)
{
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (ImGui::BeginTabItem("Fragment source"))
        {
            static ImVec4 redColor = {1,0,0,1};
            static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            
            bool headerErrors(gui_.headerErrors.size() > 0);
            bool madeReplacements = 
                gui_.sourceEditor.renderFindReplaceToolGui();
            flags_.uncompiledChanges = 
                flags_.uncompiledChanges || madeReplacements;
            if (ImGui::TreeNode("Header"))
            {
                float indent(gui_.sourceEditor.getLineIndexColumnWidth());
                ImGui::Unindent(); // Remove indent from Header TreeNode
                ImGui::Indent(indent);
                ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
                ImGui::Text(gui_.sourceHeader.c_str());
                ImGui::PopStyleColor(); 
                if 
                (
                    headerErrors && 
                    ImGui::IsItemHovered() && 
                    ImGui::BeginTooltip()
                )
                {
                    ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                    ImGui::PushStyleColor(ImGuiCol_Text, redColor);
                    ImGui::Text(gui_.headerErrors.c_str());
                    ImGui::PopStyleColor();
                    ImGui::PopTextWrapPos();
                    ImGui::EndTooltip();
                }
                ImGui::TreePop();
                ImGui::Separator();
                ImGui::Dummy(ImVec2(-1, ImGui::GetTextLineHeight()));
                ImGui::Unindent(indent);
                ImGui::Indent(); // Re-add indent from Header TreeNode
            }
            gui_.sourceEditor.renderGui("##sourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            bool madeReplacements = 
                gui_.sharedSourceEditor.renderFindReplaceToolGui();
            flags_.uncompiledChanges = 
                flags_.uncompiledChanges || madeReplacements;
            gui_.sharedSourceEditor.renderGui("##sharedSourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            Uniform::renderUniformsGui
            (
                sharedUnifoms, 
                this,
                layers,
                resources
            );
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

void Layer::resetSharedSourceEditor()
{
    Layer::GUI::sharedSourceEditor.setText(Layer::GUI::defaultSharedSource);
    Layer::GUI::sharedSourceEditor.resetTextChanged();
}

}
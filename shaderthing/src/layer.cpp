/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/gui.h"
#include "shaderthing/include/layer.h"
#include "shaderthing/include/objectio.h"
#include "shaderthing/include/uniform.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/imgui_extensions.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"
#include "thirdparty/icons/IconsFontAwesome5.h"

namespace ShaderThing
{

UPtr<vir::Shader> Layer::RenderState::textureMapperShader;

TextEditor        Layer::sharedSourceEditor_;

const std::string Layer::defaultSharedSource_ = 
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

//----------------------------------------------------------------------------//

Layer::Layer(unsigned int aId, AppState& appState) :
    id(aId), 
    imGuiMenuId("menuLayer"+std::to_string(id)),
    imGuiTabId("tabLayer"+std::to_string(id)),
    appState_(appState)
{
    name_ = "Layer "+std::to_string(id);

    auto window = vir::Window::instance();
    setResolution
    (
        {window->width(), window->height()}, 
        false
    );

    // Init quad for rendering
    setDepth((float)appState.layers.size()/Layer::nMaxLayers);

    // Init unfiorm buffer storage
    this->uniformBuffer = 
        vir::DynamicUniformBuffer::create(1024, "privateUniformBlock");
    // First two points taken by shared vertex shader uniform block and shared
    // fragment uniform block
    unsigned int bindingPoint = 2+id;
    this->uniformBufferBindingPoint = bindingPoint;
    this->uniformBuffer->setBindingPoint(bindingPoint);

    // Add default uniforms
    {
        auto& u = Uniform::create(this);
        u->managedType = Uniform::ManagedType::LayerAspectRatio;
        u->name() = "iAspectRatio";
        u->setValuePtr(&aspectRatio_, Uniform::Type::Float);
        u->gui.showBounds = false;
        renderState_.iAspectRatioUniform = u.getWeak();
    }
    {
        auto& u = Uniform::create(this);
        u->managedType = Uniform::ManagedType::LayerResolution;
        u->name() = "iResolution";
        u->setValuePtr(&resolution_, Uniform::Type::Float2);
        u->gui.bounds = glm::vec2(1.0f, 4096.0f);
        u->gui.showBounds = false;
        renderState_.iResolutionUniform = u.getWeak();
    }

    // Set default fragment source in editor
    sourceEditor_.setText
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
    sourceEditor_.resetTextChanged();

    // TODO compileShaders flag
    //if (true)
    //    compileShader();

    //if (appState.renderState.isTiledRenderingEnabled)  
    //    setRenderingTiles(appState, appState.renderState.nTiles);
}

//----------------------------------------------------------------------------//

UPtr<Layer> Layer::create(unsigned int id, AppState& appState)
{
    // Initialize shared texture mapper shader if not initialized already
    if (!Layer::RenderState::textureMapperShader.valid())
    {
        std::string vertexSource = vertexShaderSource(*appState.sharedUniforms);
        std::string fragmentSource =
            vir::Shader::currentContextShadingLanguageDirectives()+
R"(out  vec4      fragColor;
in      vec2      qc;
in      vec2      tc;
uniform sampler2D tx;
void main(){fragColor = texture(tx, tc);})";
        Layer::RenderState::textureMapperShader =
            vir::Shader::create
            (
                vertexSource,
                fragmentSource,
                vir::Shader::ConstructFrom::SourceCode
            );
        Layer::RenderState::textureMapperShader->bind();
        Layer::RenderState::textureMapperShader->bindUniformBlock
        (
            appState.sharedUniforms->vertex.uniformBuffer->name(),
            appState.sharedUniforms->vertex.uniformBufferBindingPoint
        );
        Layer::RenderState::textureMapperShader->setUniformInt("tx", 0);
    }

    auto layer = UPtr<Layer>(new Layer(id, appState));
    return layer;
}

//----------------------------------------------------------------------------//

UPtr<Layer> Layer::loadFrom
(
    ObjectIO& io, 
    unsigned int id, 
    AppState& appState
)
{
    auto layer = Layer::create(id, appState);

    layer->name_ = io.name();
    /*
    layer->flags_.rename = true; // <- hack to prevent layer tab bar re-ordering
                                 // on first renderGui after loading
    */
    layer->renderState_.target = 
        (RenderState::Target)io.read<int>("renderTarget");
    layer->resolution_ = (glm::vec2)io.read<glm::ivec2>("resolution");
    layer->aspectRatio_ = float(layer->resolution_.x)/layer->resolution_.y;
    layer->resolutionRatio_ = io.read<glm::vec2>("resolutionRatio");
    // Ensure iAspectRatio and iResolution values are actually updated
    layer->uniformBuffer->markUniformForSubmission(layer->uniforms[0].get());
    layer->uniformBuffer->markUniformForSubmission(layer->uniforms[1].get()); 
    
    layer->rescaleWithWindow_ = 
        io.readOrDefault<bool>("rescaleWithWindow", true);
    
    layer->setDepth(io.read<float>("depth"));

    auto exportData = io.readObject("exportData");
    layer->exportSettings.resolutionScale = 
        exportData.read<float>("resolutionScale");
    layer->exportSettings.rescaleWithOutput = 
        exportData.read<bool>("rescaleWithOutput");
    layer->exportSettings.windowResolutionScale = 
        exportData.read<float>("windowResolutionScale");
    layer->exportSettings.resolution =
        layer->resolution_ * 
        layer->exportSettings.resolutionScale *
        layer->exportSettings.windowResolutionScale +.5f;

    layer->isAspectRatioBoundToWindow_ = 
        io.read<bool>("isAspectRatioBoundToWindow");

    auto shaderData = io.readObject("shader");
    auto fragmentSource = shaderData.read("fragmentSource", false);

    Uniform::loadAllFrom
    (
        shaderData,
        layer.getWeak(),
        appState.resources,
        layer->cache.uninitializedResourceLayers
    );
    
    layer->sourceEditor_.setText(fragmentSource);
    layer->sourceEditor_.resetTextChanged();

    // Layer shaders not compiled here, but in loadAll, after the 
    // re-establishment of possible layer resource dependencies

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

    layer->exportSettings.clearPolicy = 
        (ExportSettings::FramebufferClearPolicy)
        framebufferData.read<int>("exportClearPolicy");

    // Initialize post-processing effects, if any were saved 
    /* TODO
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
    }*/

    //
    if (layer->renderState_.target != RenderState::Target::Window)
        addLayerToResources(layer.getWeak(), appState.resources);
    
    return layer;
}

//----------------------------------------------------------------------------//

Layer::~Layer()
{}

//----------------------------------------------------------------------------//

bool Layer::hasCompilationErrors() const
{
    return 
        sourceEditor_.getErrorMarkers().size() > 0 || 
        headerErrors_.size() > 0;
}

//----------------------------------------------------------------------------//

void Layer::setResolution
(
    glm::ivec2 resolution,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio,
    const bool setExportResolution
)
{
    static const auto* window(vir::Window::instance());
    glm::vec2 windowResolution(window->width(), window->height());
    if (windowFrameManuallyDragged)
    {
        resolution = 
            glm::max(resolutionRatio_*(glm::vec2)resolution+.5f, {1,1});
        auto viewport = Helpers::normalizedWindowResolution();
        renderState_.quad->update(viewport.x, viewport.y, depth_);
        if (!rescaleWithWindow_)
            return;
    }
    else if (!window->iconified())
        resolutionRatio_ = (glm::vec2)resolution/windowResolution;

    if (resolution == (glm::ivec2)resolution_)
        return;

    if 
    (
        tryEnfoceWindowAspectRatio &&
        isAspectRatioBoundToWindow_ &&
        !window->iconified()
    )
    {
        float windowAspectRatio = window->aspectRatio();
        if (resolution.x == (int)resolution_.x)
        {
            resolution_.x = (int)(resolution.y*windowAspectRatio+.5f);
            resolution_.y = resolution.y;
        }
        else if (resolution.y == (int)resolution_.y)
        {
            resolution_.y = (int)(resolution.x/windowAspectRatio+.5f);
            resolution_.x = resolution.x;
        }
        resolutionRatio_ = resolution_/windowResolution;
    }
    else
        resolution_ = resolution;
    aspectRatio_ = resolution_.x/resolution_.y;

    if (setExportResolution)
        exportSettings.resolution =
            resolution_*
            exportSettings.resolutionScale*
            exportSettings.windowResolutionScale + .5f;

    rebuildFramebuffers
    (
        renderState_.backFramebuffer == nullptr ?
        vir::TextureBuffer::InternalFormat::RGBA_SF_32 :
        renderState_.backFramebuffer->colorBufferInternalFormat(),
        resolution_
    );
    if (!renderState_.shader.valid())
        return;
    renderState_.shader->bind();
    if (renderState_.iAspectRatioUniform.valid())
        renderState_.iAspectRatioUniform->
            markForSubmissionToAllClientBuffers();
    if (renderState_.iResolutionUniform.valid())
        renderState_.iResolutionUniform->
            markForSubmissionToAllClientBuffers();
}

//----------------------------------------------------------------------------//

void Layer::setDepth(const float depth)
{
    depth_ = depth;
    if (renderState_.quad.valid())
        renderState_.quad->update
        (
            renderState_.quad->width(),
            renderState_.quad->height(),
            depth
        );
    else
    {
        auto viewport = Helpers::normalizedWindowResolution();
        renderState_.quad = 
            vir::makeUnique<vir::TiledQuad>(viewport.x, viewport.y, depth);
    }
}

//----------------------------------------------------------------------------//

void Layer::setFramebufferWrapMode(int i, WrapMode mode)
{
    renderState_.framebufferA->setColorBufferWrapMode(i, mode);
    renderState_.framebufferB->setColorBufferWrapMode(i, mode);
}

//----------------------------------------------------------------------------//

void Layer::setFramebufferMagFilterMode(FilterMode mode)
{
    renderState_.framebufferA->setColorBufferMagFilterMode(mode);
    renderState_.framebufferB->setColorBufferMagFilterMode(mode);
}

//----------------------------------------------------------------------------//

void Layer::setFramebufferMinFilterMode(FilterMode mode)
{
    renderState_.framebufferA->setColorBufferMinFilterMode(mode);
    renderState_.framebufferB->setColorBufferMinFilterMode(mode);
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
        UPtr<vir::Framebuffer>& framebuffer, 
        UPtr<vir::TiledQuad>& quad,
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
            Layer::RenderState::textureMapperShader->bind();
            Layer::RenderState::textureMapperShader->setUniformInt("tx", 0);
            framebuffer->bindColorBuffer(0);
            
            // This rendering step is to copy the original framebuffer contents
            // to the new framebuffer according to the original framebuffer
            // filtering options
            if (quad != nullptr)
                vir::Renderer::instance()->submit
                (
                    *quad, 
                    Layer::RenderState::textureMapperShader.get(),
                    newFramebuffer.get()
                );
            framebuffer->unbind();
            framebuffer = std::move(newFramebuffer);

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
        renderState_.framebufferA, 
        renderState_.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    rebuildFramebuffer
    (
        renderState_.framebufferB, 
        renderState_.quad, 
        internalFormat, 
        glm::max(resolution, {1,1})
    );
    renderState_.backFramebuffer = renderState_.framebufferA.get();
    renderState_.frontFramebuffer = renderState_.framebufferB.get();
    renderState_.resourceFramebuffer = 
        appState_.renderState.isTiledRenderingEnabled ?
            renderState_.frontFramebuffer :
            renderState_.backFramebuffer;
}

//----------------------------------------------------------------------------//

void Layer::clearFramebuffers()
{
    renderState_.framebufferA->clearColorBuffer();
    renderState_.framebufferB->clearColorBuffer();
}

//----------------------------------------------------------------------------//

std::string Layer::fragmentShaderSourceHeader() const
{
    std::string header =
        vir::Shader::currentContextShadingLanguageDirectives() +
        "in      vec2   qc;\nin      vec2   tc;\nout     vec4   fragColor;\n" +
        appState_.sharedStorage->shaderSource() +
        appState_.sharedUniforms->fragment.uniformBuffer->shaderSource() +
        "\n";
    unsigned int nLines = 0;
    unsigned int imageBindingPoint = 0;
    auto writeResourceUniformsToHeader = []
    (
        const UniformContainer& uniformContainer,
        std::string& header,
        unsigned int& nLines,
        unsigned int& imageBindingPoint
    )
    {
        for (auto& u : uniformContainer.uniforms)
        {
            // If the uniform has no name, I can't add it to the source
            if (u->name().size() == 0)
                continue;
            std::string uniformTypeName = 
                vir::Shader::uniformTypeToName[u->type()];
            switch (u->type())
            {
                case vir::Uniform::Type::Image2D :
                case vir::Uniform::Type::Image3D :
                case vir::Uniform::Type::ImageCube :
                {
                    auto resource = u->getValuePtr<Resource>();
                    if (resource == nullptr)
                        break;
                    header += 
                        "layout(binding="+std::to_string(imageBindingPoint++)+
                        ", "+resource->internalFormatName()+") ";
                    // This logic should be handled differently at the vir:: 
                    // level and exposed via Resource::, not here
                    if (resource->isInternalFormatUnsigned())
                        uniformTypeName = "u"+uniformTypeName;
                    header += "uniform "+uniformTypeName+" "+u->name()+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    if (u->resourceResolutionUniform().valid())
                        u->resourceResolutionUniform()->name() = u->name() +
                            "Resolution";
                    break;
                }
                case vir::Uniform::Type::Sampler2D :
                case vir::Uniform::Type::Sampler3D :
                case vir::Uniform::Type::SamplerCube :
                {
                    auto resource = u->getValuePtr<Resource>();
                    if (resource == nullptr)
                        break;
                    // This logic should be handled different at the vir:: 
                    // level and exposed via Resource::, not here
                    if (resource->isInternalFormatUnsigned())
                        uniformTypeName = "u"+uniformTypeName;
                    header += "uniform "+uniformTypeName+" "+u->name()+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    if (u->resourceResolutionUniform().valid())
                        u->resourceResolutionUniform()->name() = u->name() +
                            "Resolution";
                    break;
                }
                default :
                    break;
            }
        }
    };
    writeResourceUniformsToHeader
    (
        appState_.sharedUniforms->fragment,
        header,
        nLines,
        imageBindingPoint
    );
    writeResourceUniformsToHeader
    (
        *this,
        header,
        nLines,
        imageBindingPoint
    );
    header += this->uniformBuffer->shaderSource();
    return header;
}

//----------------------------------------------------------------------------//

std::string Layer::vertexShaderSource() const
{
    return vertexShaderSource(*appState_.sharedUniforms);
}

//----------------------------------------------------------------------------//

std::string Layer::vertexShaderSource(const SharedUniforms& sharedUniforms)
{
    std::string vertexSource
    (
        vir::Shader::currentContextShadingLanguageDirectives() +
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
)" + sharedUniforms.vertex.uniformBuffer->shaderSource() +
R"(
void main(){
    gl_Position = iMVP*vec4(iqc, 1.);
    qc = iqc.xy;
    tc = itc;})"
    );
    return vertexSource;
}

//----------------------------------------------------------------------------//

bool Layer::compileShader(bool setBlankShaderOnError)
{
    sourceHeader_ = fragmentShaderSourceHeader();
    unsigned int nHeaderLines = Helpers::countNewLines(sourceHeader_);
    unsigned int nSharedLines = sharedSourceEditor_.getTotalLines()+1;
    std::string vertexSource = vertexShaderSource();
    std::string fragmentSource = 
    (
        sourceHeader_ +
        sharedSourceEditor_.getText()+"\n"+
        sourceEditor_.getText()
    );
    auto shader = vir::Shader::create
    (
        vertexSource,
        fragmentSource,
        vir::Shader::ConstructFrom::SourceCode
    );
    if (shader->valid())
    {
        headerErrors_.clear();
        sourceEditor_.setErrorMarkers({});
        sharedSourceEditor_.setErrorMarkers({});
        cache.uncompiledUniforms.erase
        (
            std::remove_if
            (
                cache.uncompiledUniforms.begin(),
                cache.uncompiledUniforms.end(),
                [](WPtr<Uniform>& u)
                {
                    if (u.valid())
                        return u->name().size()>0;
                    else
                        return true;
                }
            ),
            cache.uncompiledUniforms.end()
        );
        hasUncompiledEdits = false;
        shader->bindUniformBlock
        (
            this->uniformBuffer->name(), 
            this->uniformBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appState_.sharedUniforms->fragment.uniformBuffer->name(),
            appState_.sharedUniforms->fragment.uniformBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appState_.sharedUniforms->vertex.uniformBuffer->name(),
            appState_.sharedUniforms->vertex.uniformBufferBindingPoint
        );
        appState_.sharedStorage->bindShader(shader.get());
        shader->bind();
        renderState_.shader = std::move(shader);
        return true;
    }
    // Else if shader not valid
    std::map<int, std::string> sourceErrors, sharedErrors;
    appState_.layersHaveCompilationErrors = true;
    for (const auto& error : shader->compilationErrors().fragmentErrors)
    {
        int sourceLineNo(error.first - nSharedLines - nHeaderLines + 1);
        int sharedLineNo(error.first - nHeaderLines);
        if (sourceLineNo > 0)
            sourceErrors.insert({sourceLineNo, error.second});
        else if (sharedLineNo > 0)
            sharedErrors.insert({sharedLineNo, error.second});
        else 
        {
            if (headerErrors_.size() > 0)
                headerErrors_ += "\n";
            headerErrors_ += "Header: " + error.second;
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
    setEditorErrors(sourceEditor_, sourceErrors);
    setEditorErrors(sharedSourceEditor_, sharedErrors);
    if (setBlankShaderOnError)
    {
        // Initialize the shader with a blank shader source if any compilation
        // errors are detected (back-end-only, the user will still see the 
        // source of the failed-compilation shader with the full list of 
        // compilation errors and markers)
        std::string vertexSource = vertexShaderSource();
        std::string fragmentSource = 
            vir::Shader::currentContextShadingLanguageDirectives() +
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})";
        renderState_.shader = 
            vir::Shader::create
            (
                vertexSource,
                fragmentSource,
                vir::Shader::ConstructFrom::SourceCode
            );
    }
    return false;
}

//------------------------------------------------------------------------------

void Layer::setRenderingTilesNumber(unsigned int nTiles)
{
    nTiles = std::max(nTiles, 1u);
    if (resolution_.x >= resolution_.y)
    {
        renderState_.tilingDirection = 
            Layer::RenderState::TilingDirection::Horizontal;
        nTiles = std::min(nTiles, (unsigned int)resolution_.x);
        renderState_.quad->update(nTiles, 1);
    }
    else
    {
        renderState_.tilingDirection = 
            Layer::RenderState::TilingDirection::Vertical;
        nTiles = std::min(nTiles, (unsigned int)resolution_.y);
        renderState_.quad->update(1, nTiles);
    }
    renderState_.nTiles = nTiles;
}

//----------------------------------------------------------------------------//

void Layer::saveTo(ObjectIO& io)
{
    io.writeObjectStart(name_.c_str());
    io.write("renderTarget", (int)renderState_.target);
    io.write("resolution", (glm::ivec2)resolution_);
    io.write("resolutionRatio", resolutionRatio_);
    io.write("isAspectRatioBoundToWindow", isAspectRatioBoundToWindow_);
    io.write("rescaleWithWindow", rescaleWithWindow_);
    io.write("depth", depth_);
    //
    io.writeObjectStart("internalFramebuffer");
    auto framebuffer = renderState_.backFramebuffer;
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
    io.write("exportClearPolicy", (int)exportSettings.clearPolicy);
    io.writeObjectEnd(); // End of internalFramebuffer
    //
    io.writeObjectStart("exportData");
    io.write("resolutionScale", exportSettings.resolutionScale);
    io.write("rescaleWithOutput", exportSettings.rescaleWithOutput);
    io.write("windowResolutionScale", exportSettings.windowResolutionScale);
    io.writeObjectEnd(); // End of exportData
    //
    io.writeObjectStart("shader");
    auto fragmentSource = sourceEditor_.getText();
    io.write
    (
        "fragmentSource",
        fragmentSource.c_str(),
        fragmentSource.size(),
        true
    );
    //
    io.writeObjectStart("uniforms");
    for(auto& u : this->uniforms)
    {
        u->saveTo(io);
    }
    io.writeObjectEnd(); // End of uniforms
    io.writeObjectEnd(); // End of shaders

    // Write post-processing effects data, if any
    // TODO
    /*
    if (layer->renderState.postProcesses.size() > 0)
    {
        io.writeObjectStart("postProcesses");
        for (auto& postProcess : renderState_.postProcesses)
            postProcess->save(io);
        io.writeObjectEnd(); // End of postProcesses
    }
    */
    io.writeObjectEnd(); // End of 'gui_.name'
}

//----------------------------------------------------------------------------//

void Layer::resetSharedSourceEditor()
{
    Layer::sharedSourceEditor_.setText(Layer::defaultSharedSource_);
    Layer::sharedSourceEditor_.resetTextChanged();
}

//----------------------------------------------------------------------------//

void Layer::resetSharedSourceEditor(const std::string& content)
{
    Layer::sharedSourceEditor_.setText(content);
    Layer::sharedSourceEditor_.resetTextChanged();
}

//----------------------------------------------------------------------------//

void Layer::renderShader
(
    vir::Framebuffer* target,
    const bool clearTarget
)
{
    auto isTiledRenderingEnabled = appState_.renderState.isTiledRenderingEnabled;
    auto tileIndex = appState_.renderState.tileIndex;
    auto flipBuffers = [this, isTiledRenderingEnabled]()
    {
        renderState_.backFramebuffer = 
            renderState_.backFramebuffer == renderState_.framebufferB.get() ? 
            renderState_.framebufferA.get() :
            renderState_.framebufferB.get();

        renderState_.frontFramebuffer = 
            renderState_.backFramebuffer == renderState_.framebufferB.get() ? 
            renderState_.framebufferA.get() :
            renderState_.framebufferB.get();

        renderState_.resourceFramebuffer = 
            isTiledRenderingEnabled ?
            renderState_.frontFramebuffer :
            renderState_.backFramebuffer;
    };

    bool allowClearTargetAndPostProcess = true;

    if (isTiledRenderingEnabled)
    {
        if (tileIndex == 0)
            flipBuffers();
        else if 
        (
            tileIndex > renderState_.nTiles-1
        )
            return; // Don't render anything
        else
            allowClearTargetAndPostProcess = false;
        if 
        (
            renderState_.tilingDirection == 
            Layer::RenderState::TilingDirection::Horizontal
        )
            renderState_.quad->selectVisibleTile
            (
                tileIndex, 
                0
            );
        else 
            renderState_.quad->selectVisibleTile
            (
                0, 
                tileIndex
            );
    }
    else
        flipBuffers();
    
    // Set sampler-type uniforms found in both this layer's uniforms as well
    // as the shared user-added uniforms
    renderState_.shader->bind();

    //TODO, set sampler/image uniforms once Resource-stuff implemented
    unsigned int textureUnit = 0;
    unsigned int imageUnit = 0;
    auto setSamplerUniforms = [this]
    (
        UniformContainer& uniformContainer,
        SharedUniforms& sharedUniforms,
        unsigned int& textureUnit,
        unsigned int& imageUnit
    )
    {
        for (auto& u : uniformContainer.uniforms)
        {
            bool isSampler
            (
                u->type() == vir::Uniform::Type::Sampler2D ||
                u->type() == vir::Uniform::Type::Sampler3D ||
                u->type() == vir::Uniform::Type::SamplerCube
            );
            bool isImage
            (
                u->type() == vir::Uniform::Type::Image2D ||
                u->type() == vir::Uniform::Type::Image3D ||
                u->type() == vir::Uniform::Type::ImageCube
            );
            if 
            (
                // TODO Check what this first condition was for
                // u->specialType != Uniform::SpecialType::None || 
                u->name().size() == 0 || !(isSampler || isImage)
            )
                continue;
            
            // Sampler or image case
            auto resource = u->getValuePtr<Resource>();
            if (resource == nullptr)
                continue;
            
            // Update resource resolution
            auto& ubo = uniformContainer.uniformBuffer;
                /*u->isSharedByUser ? 
                sharedUniforms.fragment.uniformBuffer.get() : 
                layer->uniformBuffer.get();*/
            auto rru = u->resourceResolutionUniform();
            if (!rru.valid())
                continue; // TODO log or handle
            if (rru->type() == Uniform::Type::Float2)
            {
                auto* value = rru->getValuePtr<glm::vec2>();
                if 
                (
                    value->x != resource->width() || 
                    value->y != resource->height()
                )
                {
                    value->x = resource->width();
                    value->y = resource->height();
                    ubo->markUniformForSubmission(rru.get());
                }
            }
            else if (rru->type() == Uniform::Type::Float3)
            {
                auto* value = rru->getValuePtr<glm::vec3>();
                if 
                (
                    value->x != resource->width() || 
                    value->y != resource->height() ||
                    value->z != resource->depth()
                )
                {
                    value->x = resource->width();
                    value->y = resource->height();
                    value->z = resource->depth();
                    ubo->markUniformForSubmission(rru.get());
                }
            }
            
            // When reading from your own framebuffer, you should always read
            // from the buffer to which you are NOT writing to (the back buffer
            // is the one that is always being written, so read from the front
            // one)
            if (resource->name() == name_)
            {
                vir::Framebuffer* sourceFramebuffer = 
                    renderState_.frontFramebuffer;
                // TODO Add back when postProcessing implemented
                // for (auto& postProcess : layer->renderState.postProcesses)
                // {
                //     if 
                //     (
                //         postProcess->isActive() && 
                //         postProcess->outputFramebuffer() != nullptr
                //     )
                //         sourceFramebuffer = postProcess->outputFramebuffer();
                // }
                if (isSampler)
                {
                    sourceFramebuffer->bindColorBuffer(textureUnit);
                    renderState_.shader->setUniformInt(u->name(), textureUnit++);
                }
                else if (isImage)
                {
                    sourceFramebuffer->bindColorBufferToImage
                    (
                        imageUnit, 
                        0, 
                        vir::TextureBuffer::ImageBindMode::ReadWrite
                    );
                    renderState_.shader->setUniformInt(u->name(), imageUnit++);
                }
            }
            else
            {
                if (isSampler)
                {
                    resource->bind(textureUnit);
                    renderState_.shader->setUniformInt(u->name(), textureUnit++);
                }
                else if (isImage)
                {
                    resource->bindImage
                    (
                        imageUnit, 
                        0, 
                        vir::TextureBuffer::ImageBindMode::ReadWrite
                    );
                    renderState_.shader->setUniformInt(u->name(), imageUnit++);
                }
            }
            // Set the (automatically managed) sampler2D/image2D resolution
            // uniform value. Should find a better way rather than setting this 
            // every render call
            if 
            (
                u->type() == vir::Uniform::Type::Sampler2D ||
                u->type() == vir::Uniform::Type::Image2D
            )
            {
                renderState_.shader->setUniformFloat
                (
                    u->name()+"AspectRatio", 
                    float(resource->width())/resource->height()
                );
                renderState_.shader->setUniformFloat2
                (
                    u->name()+"Resolution", 
                    {resource->width(), resource->height()}
                );
            }
            else if 
            (
                u->type() == vir::Uniform::Type::Sampler3D ||
                u->type() == vir::Uniform::Type::Image3D
            )
            {
                renderState_.shader->setUniformFloat3
                (
                    u->name()+"Resolution", 
                    {resource->width(), resource->height(), resource->depth()}
                );
            }
        }
    };
    setSamplerUniforms
    (
        appState_.sharedUniforms->fragment, 
        *(appState_.sharedUniforms), 
        textureUnit, 
        imageUnit
    );
    setSamplerUniforms
    (
        *this,
        *(appState_.sharedUniforms), 
        textureUnit, 
        imageUnit
    );

    this->uniformBuffer->submitUniforms();
    
    // Re-direct renderState & disable blending if not renderState to the window
    static auto globalRendering = vir::Renderer::instance();
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (renderState_.target != Layer::RenderState::Target::Window)
    {
        target = renderState_.backFramebuffer;
        globalRendering->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    globalRendering->submit
    (
        *renderState_.quad,
        renderState_.shader.get(), // TODO
        target,
        allowClearTargetAndPostProcess && 
        (
            clearTarget || // Or force clear if not renderState to window
            renderState_.target != Layer::RenderState::Target::Window
        )
    );
    appState_.sharedStorage->gpuMemoryBarrier();

    // Re-enable blending before either leaving or redirecting the rendered 
    // texture to the main window
    if (!blendingEnabled)
        globalRendering->setBlending(true);

    /* // TODO Post-processing
    // Apply post-processing effects, if any
    if (allowClearTargetAndPostProcess)
    {
        for (auto& postProcess : renderState.postProcesses)
            postProcess->run();
    }*/

    if 
    (
        renderState_.target != 
        Layer::RenderState::Target::InternalFramebufferAndWindow
    )
        return;

    Layer::RenderState::textureMapperShader->bind();
    renderState_.resourceFramebuffer->bindColorBuffer(0);
    Layer::RenderState::textureMapperShader->setUniformInt("tx", 0);
    globalRendering->submit
    (
        *renderState_.quad, 
        Layer::RenderState::textureMapperShader.get(),
        target0,
        allowClearTargetAndPostProcess && clearTarget
    );
}

//----------------------------------------------------------------------------//

void Layer::renderMenuItemGui()
{
    if 
    (
        ImGui::BeginMenu
        (
            ("Layer ["+name_+"]###"+imGuiMenuId).c_str()
        )
    )
    {
        const float fontSize(ImGui::GetFontSize());
        const float entryWidth(14*fontSize);
        ImGui::Text("Name                 ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", id);
        ImGui::PushItemWidth(entryWidth);
        if (ImGui::InputText(label.get(), &name_))
        {
            Helpers::enforceUniqueName
            (
                name_,
                appState_.layers,
                this
            );
        }
        ImGui::PopItemWidth();
        
        static std::map<Layer::RenderState::Target, const char*> 
        renderTargetToName
        {
            {
                Layer::RenderState::Target::InternalFramebufferAndWindow, 
                "Framebuffer & window"
            },
            {Layer::RenderState::Target::InternalFramebuffer, "Framebuffer"},
            {Layer::RenderState::Target::Window, "Window"}
        };
        ImGui::Text("Render target        ");
        ImGui::SameLine();
        ImGui::PushItemWidth(entryWidth);
        if 
        (
            ImGui::BeginCombo
            (
                "##renderingTarget", 
                renderTargetToName.at(renderState_.target)
            )
        )
        {
            for(auto entry : renderTargetToName)
            {
                if (!ImGui::Selectable(entry.second))
                    continue;
                auto target = entry.first;
                if (target != renderState_.target)
                {
                    renderState_.target = target;
                    auto thisWPtr = this->weakFromThis();
                    if (renderState_.target != Layer::RenderState::Target::Window)
                        ShaderThing::addLayerToResources
                        (
                            thisWPtr,
                            appState_.resources
                        );
                    else
                        ShaderThing::removeLayerFromResources
                        (
                            thisWPtr,
                            appState_.resources
                        );
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        if 
        (
            renderState_.target == Layer::RenderState::Target::Window || 
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
                isAspectRatioBoundToWindow_ ? 
                " " ICON_FA_LOCK " " : 
                " " ICON_FA_LOCK_OPEN " "
            )
        )
        {
            isAspectRatioBoundToWindow_ = isAspectRatioBoundToWindow_;
            if (isAspectRatioBoundToWindow_)
            {
                auto window = vir::Window::instance();
                setResolution({window->width(), window->height()}, false);
            }
        }
        if 
        (
            ImGui::IsItemHovered(ImGuiHoveredFlags_DelayNormal) && 
            ImGui::BeginTooltip()
        )
        {
            ImGui::Text(
                isAspectRatioBoundToWindow_ ?
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
        glm::ivec2 intResolution = resolution_;
        std::sprintf(label.get(), "##layer%dResolution", id);
        if (ImGui::InputInt2(label.get(), glm::value_ptr(intResolution)))
            setResolution(intResolution, false,true);
        ImGui::PopItemWidth();
        ImGui::Text("Auto-resize mode     ");
        ImGui::SameLine();
        if
        (
            ImGui::Button
            (
                rescaleWithWindow_ ?
                "Rescale on window resize" :
                "Do not auto-resize",
                {-1, 0}
            )
        )
            rescaleWithWindow_ = !rescaleWithWindow_;
        if 
        (
            renderState_.target == Layer::RenderState::Target::Window || 
            vir::Window::instance()->iconified()
        )
            ImGui::EndDisabled();
    
        if (renderState_.target != Layer::RenderState::Target::Window)
        {
            ImGui::SeparatorText("Framebuffer settings");
            renderFramebufferSettingsGui();

            // TODO
            /*
            ImGui::SeparatorText("Post-processing effects");
            int iDelete = -1;
            int iSrc = -1; 
            int iTrg = -1;
            int nPostProcesses = rendering.postProcesses.size();
            for (int i = 0; i < nPostProcesses; i++)
            {
                auto& postProcess = rendering.postProcesses[i];
                ImGui::PushID(i);
                if (ImGui::SmallButton(ICON_FA_TRASH))
                    iDelete = i;
                ImGui::SameLine();
                if (i == 0)
                    ImGui::BeginDisabled();
                if (ImGui::SmallButton(ICON_FA_ARROW_UP))
                {
                    iSrc = i;
                    iTrg = std::max(i-1, 0);
                }
                if (i == 0)
                    ImGui::EndDisabled();
                ImGui::SameLine();
                if (i == nPostProcesses-1)
                    ImGui::BeginDisabled();
                if (ImGui::SmallButton(ICON_FA_ARROW_DOWN))
                {
                    iSrc = i;
                    iTrg = std::min(i+1, nPostProcesses-1);
                }
                if (i == nPostProcesses-1)
                    ImGui::EndDisabled();
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
                    // Render post-processing effect GUI
                    if (postProcess->canRunOnDeviceInUse())
                        postProcess->renderGui();
                    else
                    {
                        ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                        ImGui::Text(postProcess->errorMessage().c_str());
                        ImGui::PopTextWrapPos();
                    }
                    ImGui::EndMenu();
                }
                ImGui::PopID();
            }
            if (iDelete != -1)
                rendering.postProcesses.erase
                (
                    rendering.postProcesses.begin() + iDelete
                );
            else if (iSrc != iTrg)
                std::swap
                (
                    rendering.postProcesses[iSrc], 
                    rendering.postProcesses[iTrg]
                );
            
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
                for (auto& postProcess : rendering.postProcesses)
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
                        auto postProcess = 
                            PostProcess::create(this, type);
                        if (postProcess != nullptr)
                            rendering.postProcesses.emplace_back
                            (
                                std::move(postProcess)
                            );
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            */
        }
        ImGui::EndMenu();
    }
    //ImGui::PopID();
}

//----------------------------------------------------------------------------//

void Layer::renderFramebufferSettingsGui()
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
                renderState_.backFramebuffer->
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
                rebuildFramebuffers(internalFormat, resolution_);
        }
        ImGui::EndCombo();
    }
    ImGui::PopItemWidth();

    //
    std::string selectedWrapModeX = "";
    std::string selectedWrapModeY = "";
    std::string selectedMagFilterMode = "";
    std::string selectedMinFilterMode = "";
    if (renderState_.backFramebuffer != nullptr)
    {
        selectedWrapModeX = vir::TextureBuffer::wrapModeToName.at
        (
            renderState_.backFramebuffer->colorBufferWrapMode(0)
        );
        selectedWrapModeY = vir::TextureBuffer::wrapModeToName.at
        (
            renderState_.backFramebuffer->colorBufferWrapMode(1)
        );
        selectedMagFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                renderState_.backFramebuffer->colorBufferMagFilterMode()
            );
        selectedMinFilterMode = 
            vir::TextureBuffer::filterModeToName.at
            (
                renderState_.backFramebuffer->colorBufferMinFilterMode()
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
        ) && renderState_.backFramebuffer != nullptr
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
        ) && renderState_.backFramebuffer != nullptr
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
        ) && renderState_.backFramebuffer != nullptr
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
        ) && renderState_.backFramebuffer != nullptr
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

void Layer::renderTabGui()
{
    static unsigned int gActiveTabId = 0;
    static unsigned int gActiveLayerId = 0;
    bool layerChanged = (gActiveLayerId != id);
    if (layerChanged)
        gActiveLayerId = id;
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (layerChanged && activeGuiTabId_ != gActiveTabId)
        {
            switch (gActiveTabId)
            {
                case 0 :
                    ImGui::SetTabItemClosed("Shared source");
                    ImGui::SetTabItemClosed("Uniforms");
                    break;
                case 1 :
                    ImGui::SetTabItemClosed("Fragment source");
                    ImGui::SetTabItemClosed("Uniforms");
                    break;
                case 2 :
                    ImGui::SetTabItemClosed("Fragment source");
                    ImGui::SetTabItemClosed("Shared source");
                    break;
            }
        }
        if (ImGui::BeginTabItem("Fragment source"))
        {
            bool headerErrors(headerErrors_.size() > 0);
            bool madeReplacements = sourceEditor_.renderFindReplaceToolGui();
            if (ImGui::TreeNode("Header"))
            {
                float indent(sourceEditor_.getLineIndexColumnWidth());
                ImGui::Unindent(); // Remove indent from Header TreeNode
                ImGui::Indent(indent);
                ImGui::PushStyleColor
                (
                    ImGuiCol_Text, 
                    ImGui::GetStyle().Colors[ImGuiCol_TextDisabled] // Gray
                );
                ImGui::Text(sourceHeader_.c_str());
                ImGui::PopStyleColor(); 
                if 
                (
                    headerErrors && 
                    ImGui::IsItemHovered() && 
                    ImGui::BeginTooltip()
                )
                {
                    ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                    ImGui::PushStyleColor(ImGuiCol_Text, {1,0,0,1}); // Red
                    ImGui::Text(headerErrors_.c_str());
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
            sourceEditor_.renderGui("##sourceEditor");
            bool madeEdits = sourceEditor_.isTextChanged();
            gActiveTabId = 0;
            ImGui::EndTabItem();
            hasUncompiledEdits = 
                hasUncompiledEdits || madeEdits || madeReplacements;
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            
            bool madeReplacements = 
                sharedSourceEditor_.renderFindReplaceToolGui();
            bool madeEdits = sharedSourceEditor_.isTextChanged();
            gActiveTabId = 1;
            ImGui::EndTabItem();
            hasUncompiledEdits = 
                hasUncompiledEdits || madeEdits || madeReplacements;
            sharedSourceEditor_.renderGui("##sharedSourceEditor");
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            GUI::renderUniformsTab
            (
                this,
                appState_
            );
            gActiveTabId = 2;
            ImGui::EndTabItem();
        }
        activeGuiTabId_ = gActiveTabId;
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderSharedCompilationErrorsGui()
{
    const auto& sharedSourceErrors(sharedSourceEditor_.getErrorMarkers());
    if (sharedSourceErrors.size() > 0)
    {
        ImGui::Bullet(); ImGui::Text("Shared source");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            for (auto& error : sharedSourceErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
}

//----------------------------------------------------------------------------//

void Layer::renderCompilationErrorsGui()
{
    const auto& sourceErrors(sourceEditor_.getErrorMarkers());
    if (sourceErrors.size() > 0 || headerErrors_.size() > 0)
    {
        ImGui::Bullet(); ImGui::Text(name_.c_str());
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            if (headerErrors_.size() > 0)
                ImGui::Text(headerErrors_.c_str());
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
}

//----------------------------------------------------------------------------//

}
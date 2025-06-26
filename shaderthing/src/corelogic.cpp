#include "shaderthing/include/bytedata.h"
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/helpers.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/oo/texteditor.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

std::string assembleFragmentShaderHeader
(
    const Layer& layer, 
    const AppData& appData
)
{
    std::string header =
        vir::Shader::currentContextShadingLanguageDirectives() +
        "in      vec2   qc;\nin      vec2   tc;\nout     vec4   fragColor;\n" +
        appData.sharedStorage.shaderSource() +
        appData.sharedUniforms.fBuffer->shaderSource() +
        "\n";
    /*
    unsigned int imageBindingPoint = 0;
    auto writeResourceUniformsToHeader = []
    (
        const std::vector<vir::UniquePtr<Uniform>>& uniforms, 
        std::string& header,
        unsigned int& nLines,
        unsigned int& imageBindingPoint
    )
    {
        for (auto& u : uniforms)
        {
            // If the uniform has no name, I can't add it to the source
            if (u->name.size() == 0)
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
                    // This logic should be handled different at the vir:: 
                    // level and exposed via Resource::, not here
                    if (resource->isInternalFormatUnsigned())
                        uniformTypeName = "u"+uniformTypeName;
                    header += "uniform "+uniformTypeName+" "+u->name+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    u->updateResourceResolutionName();
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
                    header += "uniform "+uniformTypeName+" "+u->name+";\n";
                    ++nLines;
                    // Also update name of linked resolution uniform
                    u->updateResourceResolutionName();
                    break;
                }
                default :
                    break;
            }
        }
    };
    writeResourceUniformsToHeader
    (
        sharedUniforms.userUniforms(),
        header,
        nLines,
        imageBindingPoint
    );
    writeResourceUniformsToHeader
    (
        uniforms_,
        header,
        nLines,
        imageBindingPoint
    );
    */
    header += layer.renderer.uniformBuffer->shaderSource();
    return header;
}

//----------------------------------------------------------------------------//

std::string assembleVertexShaderSource(const AppData& appData)
{
    std::string vertexSource
    (
        vir::Shader::currentContextShadingLanguageDirectives() +
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
)" + appData.sharedUniforms.vBuffer->shaderSource() +
R"(
void main(){
    gl_Position = iMVP*vec4(iqc, 1.);
    qc = iqc.xy;
    tc = itc;})"
    );
    return vertexSource;
}

//----------------------------------------------------------------------------//

bool compileShader(Layer& layer, AppData& appData, bool setBlankShaderOnError)
{
    layer.sourceHeader = assembleFragmentShaderHeader(layer, appData);
    unsigned int nHeaderLines = Helpers::countNewLines(layer.sourceHeader);
    unsigned int nSharedLines = appData.sharedSourceEditor.getTotalLines()+1;
    std::string vertexSource = assembleVertexShaderSource(appData);
    std::string fragmentSource = 
        (
            layer.sourceHeader +
            appData.sharedSourceEditor.getText()+"\n"+
            layer.sourceEditor.getText()
        );
    auto shader = vir::Shader::create
    (
        vertexSource,
        fragmentSource,
        vir::Shader::ConstructFrom::SourceCode
    );
    if (shader->valid())
    {
        //delete rendering_.shader;
        layer.headerErrors.clear();
        layer.sourceEditor.setErrorMarkers({});
        appData.sharedSourceEditor.setErrorMarkers({});
        layer.cache.uncompiledUniforms.erase
        (
            std::remove_if
            (
                layer.cache.uncompiledUniforms.begin(),
                layer.cache.uncompiledUniforms.end(),
                [](auto& u){return u->name.size()>0;}
            ),
            layer.cache.uncompiledUniforms.end()
        );
        layer.flags.uncompiledChanges = false;
        shader->bindUniformBlock
        (
            layer.renderer.uniformBuffer->name(), 
            layer.renderer.uniformBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appData.sharedUniforms.fBuffer->name(),
            appData.sharedUniforms.fBufferBindingPoint
        );
        shader->bindUniformBlock
        (
            appData.sharedUniforms.vBuffer->name(),
            appData.sharedUniforms.vBufferBindingPoint
        );
        appData.sharedStorage.bindShader(shader.get());
        shader->bind();
        layer.renderer.shader = std::move(shader);
        return true;
    }
    // Else if shader not valid
    std::map<int, std::string> sourceErrors, sharedErrors;
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
            if (layer.headerErrors.size() > 0)
                layer.headerErrors += "\n";
            layer.headerErrors += "Header: " + error.second;
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
    setEditorErrors(layer.sourceEditor, sourceErrors);
    setEditorErrors(appData.sharedSourceEditor, sharedErrors);
    if (setBlankShaderOnError)
    {
        // Initialize the shader with a blank shader source if any compilation
        // errors are detected (back-end-only, the user will still see the 
        // source of the failed-compilation shader with the full list of 
        // compilation errors and markers)
        std::string vertexSource = assembleVertexShaderSource(appData);
        std::string fragmentSource = 
            vir::Shader::currentContextShadingLanguageDirectives() +
R"(out vec4 fragColor;
in     vec2 qc;
in     vec2 tc;
void main(){fragColor = vec4(0, 0, 0, .5);})";
        layer.renderer.shader = 
            vir::Shader::create
            (
                vertexSource,
                fragmentSource,
                vir::Shader::ConstructFrom::SourceCode
            );
    }
    return false;
}

//----------------------------------------------------------------------------//

void createNewLayer(AppData& appData, bool compileShader)
{
    // Create and new layer to layers
    unsigned int id = Helpers::findSmallestFreeLayerId(appData.layers);
    auto& layer = *appData.layers.emplace_back(vir::makeUnique<Layer>(id));
    layer.name = "Layer "+std::to_string(id);

    // Init quad for rendering
    setLayerDepth(layer, (float)appData.layers.size()/Layer::nMaxLayers);

    // Initi unfiorm buffer storage
    layer.renderer.uniformBuffer = 
        vir::DynamicUniformBuffer::create(1024, "privateUniformBlock");
    // First two points taken by shared vertex shader uniform block and shared
    // fragment uniform block
    unsigned int bindingPoint = 2+id;
    layer.renderer.uniformBufferBindingPoint = bindingPoint;
    layer.renderer.uniformBuffer->setBindingPoint(bindingPoint);

    // Set default fragment source in editor
    layer.sourceEditor.setText
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
    layer.sourceEditor.resetTextChanged();

    if (compileShader)
        ShaderThing::compileShader(layer, appData);
}

//----------------------------------------------------------------------------//

void initialize(AppData& appData)
{
    // ImGui setup
    ImGuiIO& io = ImGui::GetIO();
    // Do not save config to .ini file
    io.IniFilename = NULL;
    io.ConfigDockingTransparentPayload = true;
    // Custom tab bar color styling
    auto scaleColor = [](unsigned int cid, float s)
    {
        auto& style = ImGui::GetStyle();
        ImVec4& c = style.Colors[cid];
        c.x*=s;
        c.y*=s;
        c.z*=s;
        c.w*=s;
    };
    scaleColor(ImGuiCol_Tab, .8);
    scaleColor(ImGuiCol_TabActive, 1.05);
    scaleColor(ImGuiCol_TabHovered, 1.05);
    
    // Font setup
    auto& font = appData.font;
    float baseFontSize = 26.f;
    font.imFontConfig.PixelSnapH = true;
    font.imFontConfig.OversampleV = 3.0;
    font.imFontConfig.OversampleH = 3.0;
    font.imFontConfig.RasterizerMultiply = 1.0;
    // The 26-36.5 ratio between Western writing systems' characters and
    // Asian logograms/characters is set so that the latter are (almost)
    // exactly twice as wide as the former, for readability, valid for the
    // selected fonts at hand
    font.imFont = io.Fonts->AddFontFromMemoryCompressedTTF
    (
        (void*)ByteData::Font::CousineRegularData,
        ByteData::Font::CousineRegularSize, 
        baseFontSize,
        &font.imFontConfig,
        io.Fonts->GetGlyphRangesDefault()
    );
    font.imFontConfig.MergeMode = true;
    font.imFontConfig.RasterizerMultiply = 1.25;
    io.Fonts->AddFontFromMemoryCompressedTTF
    (
        (void*)ByteData::Font::CousineRegularData, 
        ByteData::Font::CousineRegularSize,
        baseFontSize,
        &font.imFontConfig,
        io.Fonts->GetGlyphRangesCyrillic()
    );
    io.Fonts->AddFontFromMemoryCompressedTTF
    (
        (void*)ByteData::Font::CousineRegularData, 
        ByteData::Font::CousineRegularSize,
        baseFontSize,
        &font.imFontConfig,
        io.Fonts->GetGlyphRangesGreek()
    );

    // Font icons from FontAwesome5 (free)
    float iconFontSize = baseFontSize*2.f/3.f;
    static const ImWchar iconRanges[] = {ICON_MIN_FA, ICON_MAX_16_FA, 0};
    ImFontConfig iconConfig; 
    iconConfig.MergeMode = true; 
    iconConfig.PixelSnapH = true; 
    iconConfig.GlyphMinAdvanceX = iconFontSize;
    io.Fonts->AddFontFromMemoryCompressedTTF
    ( 
        (void*)ByteData::Font::FontAwesome5FreeSolid900Data, 
        ByteData::Font::FontAwesome5FreeSolid900Size, 
        iconFontSize,
        &iconConfig, 
        iconRanges
    );
    io.Fonts->Build();
    font.imFont->Scale = 0.6;
    font.scale = &font.imFont->Scale;

    // Set window icon
    auto window = vir::Window::instance();
    window->setIcon
    (
        (unsigned char*)ByteData::Icon::sTIconData,
        ByteData::Icon::sTIconSize,
        false
    );

    initializeSharedUniforms(appData);

    // Initialize shared texture mapper shader
    std::string vertexSource = assembleVertexShaderSource(appData);
    std::string fragmentSource =
        vir::Shader::currentContextShadingLanguageDirectives()+
R"(out  vec4      fragColor;
in      vec2      qc;
in      vec2      tc;
uniform sampler2D tx;
void main(){fragColor = texture(tx, tc);})";
    appData.renderer.textureMapperShader =
        vir::Shader::create
        (
            vertexSource,
            fragmentSource,
            vir::Shader::ConstructFrom::SourceCode
        );
    // These might not even be needed...
    /*
    appData.renderer.textureMapperShader->bindUniformBlock
    (
        appData.sharedUniforms.fBuffer->name(),
        appData.sharedUniforms.fBufferBindingPoint
    );
    appData.renderer.textureMapperShader->bindUniformBlock
    (
        appData.sharedUniforms.vBuffer->name(),
        appData.sharedUniforms.vBufferBindingPoint
    );
    */

    // Create default layer
    createNewLayer(appData);
};

//----------------------------------------------------------------------------//

void initializeSharedUniforms(AppData& appData)
{
    SharedUniforms& su = appData.sharedUniforms;

    // Init CPU block data
    static const auto window = vir::Window::instance();
    if (!window->iconified())
       su.iResolution = {window->width(), window->height()};
    su.iAspectRatio = su.iResolution.x/su.iResolution.y;
    for (int i=0; i<256; i++)
        su.iKeyboard[i] = glm::ivec3({0,0,0});

    // Init cameras
    su.screenCamera = vir::makeUnique<vir::Camera>();
    su.shaderCamera = vir::makeUnique<vir::InputCamera>();
    su.screenCamera->setProjectionType
    (
        vir::Camera::ProjectionType::Orthographic
    );
    su.screenCamera->setViewportHeight
    (
        std::min(1.0f, 1.0f/su.iAspectRatio)
    );
    su.screenCamera->setPosition({0, 0, 1});
    su.screenCamera->setPlanes(.01f, 100.f);
    su.shaderCamera->setZPlusIsLookDirection(true);
    su.shaderCamera->setDirection(su.iLook);
    su.shaderCamera->setPosition(su.iWASD);
    su.screenCamera->update();
    su.shaderCamera->update();
    //iMVP_ = screenCamera_->projectionViewMatrix();

    // Init random number generator and set initial random number
    //if (random_ == nullptr)
    //    random_ = new Random();
    //iRandom_ = random_->generateFloat();

    // Init uniform buffers, bind to designated binding points and set
    // initial data
    su.vBuffer = 
            vir::DynamicUniformBuffer::create(64, "vertexSharedUniformBlock");
    su.vBuffer->bind();
    su.vBuffer->setBindingPoint(su.vBufferBindingPoint);

    su.iMVPUniform = vir::makeUnique<Uniform>();
    su.iMVPUniform->name = "iMVP";
    su.iMVPUniform->setValuePtr
    (
        &(su.screenCamera->projectionViewMatrix()), 
        Uniform::Type::Mat4
    );
    su.iMVPUniform->gui.showBounds = false;
    su.vBuffer->addUniform(su.iMVPUniform);

    su.fBuffer = 
            vir::DynamicUniformBuffer::create(8196, "sharedUniformBlock");
    su.fBuffer->bind();
    su.fBuffer->setBindingPoint(su.fBufferBindingPoint);

    // Init uniform wrappers
    su.iFrameUniform = vir::makeUnique<Uniform>();
    su.iFrameUniform->name = "iFrame";
    su.iFrameUniform->setValuePtr(&appData.renderer.frame, Uniform::Type::Int);
    su.iFrameUniform->gui.showBounds = false;
    //su.iFrameUniform->specialType = Uniform::SpecialType::Frame;
    su.fBuffer->addUniform(su.iFrameUniform);

    su.iRenderPassUniform = vir::makeUnique<Uniform>();
    su.iRenderPassUniform->name = "iRenderPass";
    su.iRenderPassUniform->setValuePtr
    (
        &appData.renderer.renderPass, 
        Uniform::Type::Int
    );
    su.iRenderPassUniform->gui.showBounds = false;
    //su.iRenderPassUniform->specialType = Uniform::SpecialType::RenderPass;
    su.fBuffer->addUniform(su.iRenderPassUniform);
    
    su.iTimeUniform = vir::makeUnique<Uniform>();
    su.iTimeUniform->name = "iTime";
    su.iTimeUniform->setValuePtr(&su.iTime, Uniform::Type::Float);
    //su.iTimeUniform->specialType = Uniform::SpecialType::Time;
    su.fBuffer->addUniform(su.iTimeUniform);
    
    su.iTimeDeltaUniform = vir::makeUnique<Uniform>();
    su.iTimeDeltaUniform->name = "iTimeDelta";
    su.iTimeDeltaUniform->setValuePtr(&su.iTimeDelta, Uniform::Type::Float);
    su.iTimeDeltaUniform->gui.showBounds = false;
    su.fBuffer->addUniform(su.iTimeDeltaUniform);

    su.iRandomUniform = vir::makeUnique<Uniform>();
    su.iRandomUniform->name = "iRandom";
    su.iRandomUniform->setValuePtr(&su.iRandom, Uniform::Type::Float);
    su.iRandomUniform->gui.showBounds = false;
    su.fBuffer->addUniform(su.iRandomUniform);

    su.iUserActionUniform = vir::makeUnique<Uniform>();
    su.iUserActionUniform->name = "iUserAction";
    su.iUserActionUniform->setValuePtr(&su.iUserAction, Uniform::Type::Bool);
    su.iUserActionUniform->gui.showBounds = false;
    //su.iUserActionUniform->specialType = Uniform::SpecialType::UserAction;
    su.fBuffer->addUniform(su.iUserActionUniform);

    su.iExportUniform = vir::makeUnique<Uniform>();
    su.iExportUniform->name = "iExport";
    su.iExportUniform->setValuePtr
    (
        &appData.exporter.isActive, 
        Uniform::Type::Bool
    );
    su.iExportUniform->gui.showBounds = false;
    su.fBuffer->addUniform(su.iExportUniform);

    su.iWASDUniform = vir::makeUnique<Uniform>();
    su.iWASDUniform->name = "iWASD";
    su.iWASDUniform->setValuePtr(&su.iWASD, Uniform::Type::Float3);
    //su.iWASDUniform->specialType = Uniform::SpecialType::CameraPosition;
    su.fBuffer->addUniform(su.iWASDUniform);

    su.iLookUniform = vir::makeUnique<Uniform>();
    su.iLookUniform->name = "iLook";
    su.iLookUniform->setValuePtr(&su.iLook, Uniform::Type::Float3);
    su.iLookUniform->gui.showBounds = false;
    //su.iLookUniform->specialType = Uniform::SpecialType::CameraDirection;
    su.fBuffer->addUniform(su.iLookUniform);

    su.iMouseUniform = vir::makeUnique<Uniform>();
    su.iMouseUniform->name = "iMouse";
    su.iMouseUniform->setValuePtr(&su.iMouse, Uniform::Type::Float4);
    su.iMouseUniform->gui.showBounds = false;
    //su.iMouseUniform->specialType = Uniform::SpecialType::Mouse;
    su.fBuffer->addUniform(su.iMouseUniform);

    su.iAspectRatioUniform = vir::makeUnique<Uniform>();
    su.iAspectRatioUniform->name = "iWindowAspectRatio";
    su.iAspectRatioUniform->setValuePtr(&su.iAspectRatio, Uniform::Type::Float);
    su.iAspectRatioUniform->gui.showBounds = false;
    //su.iAspectRatioUniform->specialType = Uniform::SpecialType::WindowAspectRatio;
    su.fBuffer->addUniform(su.iAspectRatioUniform);

    su.iResolutionUniform = vir::makeUnique<Uniform>();
    su.iResolutionUniform->name = "iWindowResolution";
    su.iResolutionUniform->setValuePtr(&su.iResolution, Uniform::Type::Float2);
    su.iResolutionUniform->gui.showBounds = false;
    //su.iResolutionUniform->specialType = Uniform::SpecialType::WindowResolution;
    su.fBuffer->addUniform(su.iResolutionUniform);

    su.iKeyboardUniform = vir::makeUnique<Uniform>();
    su.iKeyboardUniform->name = "iKeyboard";
    su.iKeyboardUniform->setValuePtr(&su.iKeyboard, Uniform::Type::Int3, 256);
    su.iKeyboardUniform->gui.showBounds = false;
    //su.iKeyboardUniform->specialType = Uniform::SpecialType::Keyboard;
    su.fBuffer->addUniform(su.iKeyboardUniform);
}

//----------------------------------------------------------------------------//

void preRenderUpdate(AppData& appData)
{
    appData.deferredActionBuffer.process();
}

//----------------------------------------------------------------------------//

void postRenderUpdate(AppData& appData)
{
    
}

//----------------------------------------------------------------------------//

RenderResult renderShaders(AppData& appData)
{
    return {true, true};
}

//----------------------------------------------------------------------------//

void setupNewProject(AppData& appData)
{
    appData.layers.clear();
    initializeSharedUniforms(appData);
    createNewLayer(appData);
}

//----------------------------------------------------------------------------//

void setLayerDepth(Layer& layer, const float depth)
{
    layer.depth = depth;
    if (layer.renderer.quad.valid())
        layer.renderer.quad->update
        (
            layer.renderer.quad->width(),
            layer.renderer.quad->height(),
            depth
        );
    else
    {
        auto viewport = Helpers::normalizedWindowResolution();
        layer.renderer.quad = 
            vir::makeUnique<vir::TiledQuad>(viewport.x, viewport.y, depth);
    }
}

//----------------------------------------------------------------------------//

void setLayerFramebufferWrapMode(Layer& layer, int i, WrapMode mode)
{
    layer.renderer.framebufferA->setColorBufferWrapMode(i, mode);
    layer.renderer.framebufferB->setColorBufferWrapMode(i, mode);
}

//----------------------------------------------------------------------------//

void setLayerFramebufferMagFilterMode(Layer& layer, FilterMode mode)
{
    layer.renderer.framebufferA->setColorBufferMagFilterMode(mode);
    layer.renderer.framebufferB->setColorBufferMagFilterMode(mode);
}

//----------------------------------------------------------------------------//

void setLayerFramebufferMinFilterMode(Layer& layer, FilterMode mode)
{
    layer.renderer.framebufferA->setColorBufferMinFilterMode(mode);
    layer.renderer.framebufferB->setColorBufferMinFilterMode(mode);
}

//----------------------------------------------------------------------------//

void rebuildLayerFramebuffers
(
    Layer& layer,
    const vir::TextureBuffer::InternalFormat& internalFormat, 
    const glm::ivec2& resolution,
    const AppData& appData
)
{
    auto& renderer = layer.renderer;
    auto rebuildFramebuffer = []
    (
        UPtr<vir::Framebuffer>& framebuffer, 
        UPtr<vir::TiledQuad>& quad,
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution,
        const UPtr<vir::Shader>& textureMapper
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
            textureMapper->bind();
            textureMapper->setUniformInt("tx", 0);
            framebuffer->bindColorBuffer(0);
            
            // This rendering step is to copy the original framebuffer contents
            // to the new framebuffer according to the original framebuffer
            // filtering options
            if (quad != nullptr)
                vir::Renderer::instance()->submit
                (
                    *quad, 
                    textureMapper.get(),
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
        renderer.framebufferA, 
        renderer.quad, 
        internalFormat, 
        glm::max(resolution, {1,1}),
        appData.renderer.textureMapperShader
    );
    rebuildFramebuffer
    (
        renderer.framebufferB, 
        renderer.quad, 
        internalFormat, 
        glm::max(resolution, {1,1}),
        appData.renderer.textureMapperShader
    );
    renderer.backFramebuffer = renderer.framebufferA.get();
    renderer.frontFramebuffer = renderer.framebufferB.get();
    renderer.resourceFramebuffer = 
        appData.renderer.isTiledRenderingEnabled ?
            renderer.frontFramebuffer :
            renderer.backFramebuffer;
}

//----------------------------------------------------------------------------//

void clearLayerFramebuffers(Layer& layer)
{
    layer.renderer.framebufferA->clearColorBuffer();
    layer.renderer.framebufferB->clearColorBuffer();
}

//----------------------------------------------------------------------------//

}
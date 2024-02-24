#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/shareduniforms.h"
#include "shaderthing-p/include/uniform.h"
#include "shaderthing-p/include/resource.h"
#include "shaderthing-p/include/helpers.h"
#include "shaderthing-p/include/macros.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

TextEditor Layer::GUI::sharedSourceEditor = TextEditor();
bool       Layer::Flags::restartRendering = false;

//----------------------------------------------------------------------------//

Layer::Layer
(
    const std::vector<Layer*>& layers,
    const SharedUniforms& sharedUniforms
) :
    id_(findFreeId(layers))
{
    // Set layer resolution from current app window resolution
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    resolution_ = {window->width(), window->height()};
    resolutionRatio_ = {1,1};
    aspectRatio_ = ((float)resolution_.x)/resolution_.y;

    // Add default uniforms
    {
        Uniform* u = nullptr;
        
        u = new Uniform();
        u->specialType = Uniform::SpecialType::LayerAspectRatio;
        u->name = "iAspectRatio";
        u->type = Uniform::Type::Float;
        u->setValuePtr(&aspectRatio_);
        u->gui.showBounds = false;
        uniforms_.emplace_back(u);

        u = new Uniform();
        u->specialType = Uniform::SpecialType::LayerResolution;
        u->name = "iResolution";
        u->type = Uniform::Type::Float2;
        u->setValuePtr(&resolution_);
        u->gui.bounds = glm::vec2(1.0f, 4096.0f);
        u->gui.showBounds = false;
        uniforms_.emplace_back(u);
    };

    // Set name
    gui_.name = "Layer "+std::to_string(id_);
    gui_.newName = gui_.name;

    // Set default fragment source in editor
    gui_.sourceEditor.setText
    (
R"(void main()
{
    fragColor = vec4
    (
        .5+.25*sin(2*(qc+iTime)),
        .75,
        1.
    );
})"
    );
    gui_.sourceEditor.resetTextChanged();

    // Compile shader
    compileShader(sharedUniforms);

    // Set depth (also inits rendering quad on first call)
    setDepth((float)layers.size()/Layer::nMaxLayers);

    // Init framebuffers
    rebuildFramebuffers
    (
        vir::TextureBuffer2D::InternalFormat::RGBA_SF_32,
        resolution_
    );

    // Register with event broadcaster
    this->tuneIntoEventBroadcaster(VIR_DEFAULT_PRIORITY+id_);
}

//----------------------------------------------------------------------------//

Layer::~Layer()
{
    DELETE_IF_NOT_NULLPTR(rendering_.framebufferA)
    DELETE_IF_NOT_NULLPTR(rendering_.framebufferB)
    DELETE_IF_NOT_NULLPTR(rendering_.shader)
    DELETE_IF_NOT_NULLPTR(rendering_.quad)
}

//----------------------------------------------------------------------------//

void Layer::onReceive(vir::Event::WindowResizeEvent& event)
{
    glm::ivec2 resolution = {event.width, event.height};
    setResolution(resolution, true);
}

//----------------------------------------------------------------------------//

const unsigned int Layer::findFreeId(const std::vector<Layer*>& layers)
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
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
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
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    static const std::string vertexSource
    (
        glslVersionSource()+
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
)" + sharedUniforms.glslBlockSource() +
R"(
uniform mat4 iMVP2;
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
    int nLines = 16;
    std::string header
    (
        glslVersionSource()+
        "in vec2 qc;\nin vec2 tc;\nout vec4 fragColor;\n"+
        sharedUniforms.glslBlockSource()
    );

    for (const auto* u : uniforms_)
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
        // Automatically managed sampler2D resolution uniform
        if (isSampler2D)
        {
            header +=
                "uniform vec2 "+u->name+"Resolution;\n";
            ++nLines;
        }
    }

    return {header, nLines};
}

//----------------------------------------------------------------------------//

void Layer::setResolution
(
    glm::ivec2& resolution,
    const bool windowFrameManuallyDragged,
    const bool tryEnfoceWindowAspectRatio
)
{
    static const auto* window(vir::GlobalPtr<vir::Window>::instance());
    glm::vec2 windowResolution(window->width(), window->height());
    if (windowFrameManuallyDragged)
    {
        resolution = 
            glm::max(resolutionRatio_*(glm::vec2)resolution+.5f, {1,1});
        auto viewport = Helpers::normalizedWindowResolution();
        rendering_.quad->update(viewport.x, viewport.y, depth_);
    }
    else
        resolutionRatio_ = (glm::vec2)resolution/windowResolution;
    
    if (resolution == resolution_)
        return;
    
    if (tryEnfoceWindowAspectRatio && flags_.windowBoundAspectRatio)
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
        //if (windowFrameManuallyDragged)
        //    resolution = 
        //        glm::max(resolutionRatio_*(glm::vec2)resolution+.5f, {1,1});
    }
    else
        resolution_ = resolution;
    aspectRatio_ = ((float)resolution_.x)/resolution_.y;
    rebuildFramebuffers
    (
        rendering_.framebuffer->colorBufferInternalFormat(),
        resolution_
    );
    rendering_.shader->bind();
    rendering_.shader->setUniformFloat("iAspectRatio", aspectRatio_);
    rendering_.shader->setUniformFloat2("iResolution", (glm::vec2)resolution_);
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
            framebuffer->unbind();
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
        }
        else
            framebuffer = vir::Framebuffer::create
            (
                resolution.x, 
                resolution.y, 
                internalFormat
            );
    };
    rebuildFramebuffer(rendering_.framebufferA, internalFormat, resolution);
    rebuildFramebuffer(rendering_.framebufferB, internalFormat, resolution);
    rendering_.framebuffer = rendering_.framebufferA;
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
        flags_.uncompiledChanges = false;
        // Re-set uniforms
        sharedUniforms.bindShader(rendering_.shader);
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
        return true;
    }
    // Else if shader not valid
    std::map<int, std::string> sourceErrors, sharedErrors;
    for (const auto error : shader->compilationErrors().fragmentErrors)
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
    // Set sampler-type uniforms
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
        if (resource->name() == gui_.name)
        {
            // Buffers are flipped afterwards, so rendering_.framebuffer
            // is guaranteed to contain the read-only framebuffer pointer
            vir::Framebuffer* sourceFramebuffer = rendering_.framebuffer;
            /*for (auto* postProcess : postProcesses_)
            {
                if 
                (
                    postProcess->isActive() && 
                    postProcess->outputFramebuffer() != nullptr
                )
                    sourceFramebuffer = postProcess->outputFramebuffer();
            }*/
            sourceFramebuffer->bindColorBuffer(unit);
        }
        else
            resource->bind(unit);
        rendering_.shader->setUniformInt(u->name, unit);
        unit++;
        // Set the (automatically managed) sampler2D resolution uniform value
        if (u->type == vir::Shader::Variable::Type::Sampler2D)
            rendering_.shader->setUniformFloat2
            (
                u->name+"Resolution", 
                {resource->width(), resource->height()}
            );
    }

    // Flip buffers
    rendering_.framebuffer = 
        rendering_.framebuffer == rendering_.framebufferB ? 
        rendering_.framebufferA :
        rendering_.framebufferB;

    // Set all uniforms that are not set in the GUI step
    // setSharedDefaultSamplerUniforms();
    
    // Re-direct rendering & disable blending if not rendering to the window
    static auto renderer = vir::GlobalPtr<vir::Renderer>::instance();
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (rendering_.target != Layer::Rendering::Target::Window)
    {
        target = rendering_.framebuffer;
        renderer->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    rendering_.shader->bind();
    renderer->submit
    (
        *rendering_.quad,
        rendering_.shader,
        target,
        clearTarget || // Or force clear if not rendering to window
        rendering_.target != Layer::Rendering::Target::Window
    );

    // Re-enable blending before either leaving or redirecting the rendered 
    // texture to the main window
    if (!blendingEnabled)
        vir::GlobalPtr<vir::Renderer>::instance()->setBlending(true);

    // Apply post-processing effects, if any
    // for (auto postProcess : postProcesses_)
    //    postProcess->run();

    if 
    (
        rendering_.target != 
        Layer::Rendering::Target::InternalFramebufferAndWindow
    )
        return;

    // Render texture rendered by the previous call to the provided initial 
    // target (or to main window if target0 == nullptr). Also, manage the
    // lifetime of the shared internal framebuffer via a static unique_ptr
    static auto internalFramebufferShader
    (
        std::unique_ptr<vir::Shader>
        (
            vir::Shader::create
            (
                vertexShaderSource(sharedUniforms),
                glslVersionSource()+
R"(out  vec4      fragColor;
in      vec2      qc;
in      vec2      tc;
uniform sampler2D self;
void main(){fragColor = texture(self, tc);})",
                vir::Shader::ConstructFrom::SourceCode
            )
        )
    );
    internalFramebufferShader->bind();
    rendering_.framebuffer->bindColorBuffer(0);
    internalFramebufferShader->setUniformInt("self", 0);
    renderer->submit
    (
        *rendering_.quad, 
        internalFramebufferShader.get(), 
        target0,
        clearTarget
    );
}

//----------------------------------------------------------------------------//

bool Layer::removeResourceFromUniforms(const Resource* resource)
{
    for (int i=0; i<uniforms_.size(); i++)
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

void Layer::renderShaders // Static
(
    const std::vector<Layer*>& layers,
    vir::Framebuffer* target, 
    const SharedUniforms& sharedUniforms
)
{
    bool clearTarget(true);
    for (auto layer : layers)
    {
        layer->renderShader(target, clearTarget, sharedUniforms);
        // At the end of this loop, the status of clearTarget will represent
        // whether the main window has been cleared of its contents at least
        // once (true if never cleared at least once)
        if 
        (
            clearTarget &&
            layer->rendering_.target != 
                Layer::Rendering::Target::InternalFramebuffer
        )
            clearTarget = false;
    }
    // If the window has not been cleared at least once, or if I am not
    // rendering to the window at all (i.e., if target != nullptr, which is
    // only true during exports), then render a dummy/void/blank window,
    // simply to avoid visual artifacts when nothing is rendering to the
    // main window. As for the internalFramebufferShader in Layer::renderShader,
    // the lifetime of the shader (and quad) is managed statically within here
    if (clearTarget || target != nullptr)
    {
        static std::unique_ptr<vir::Quad> blankQuad(new vir::Quad(1, 1, 0));
        auto viewport = Helpers::normalizedWindowResolution();
        blankQuad->update(viewport.x, viewport.y, 0);
        static auto blankShader
        (
            std::unique_ptr<vir::Shader>
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
            )
        );
        vir::GlobalPtr<vir::Renderer>::instance()->submit
        (
            *blankQuad.get(), 
            blankShader.get()
        );
    }
}

//----------------------------------------------------------------------------//

void Layer::renderSettingsMenuGUI(std::vector<Resource*>& resources)
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
                                  // function Layer::renderLayersTabBarGUI(...)
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
                        Resource::removeFramebufferFromResources
                        (
                            &(rendering_.framebuffer),
                            resources
                        );
                    else
                        Resource::insertFramebufferInResources
                        (
                            &(gui_.name),
                            &(rendering_.framebuffer),
                            resources
                        );
                }
            }
            ImGui::EndCombo();
        }
        ImGui::PopItemWidth();

        ImGui::Text("Resolution           ");
        ImGui::SameLine();
        if (rendering_.target == Rendering::Target::Window)
            ImGui::BeginDisabled();
        ImGui::SameLine();
        auto x0 = ImGui::GetCursorPos().x;
        std::sprintf(label.get(), "##layer%dResolution", id_);
        if 
        (
            ImGui::Checkbox
            (
                label.get(), 
                &flags_.windowBoundAspectRatio
            )
        )
        {
            if (flags_.windowBoundAspectRatio)
            {
                auto window = vir::GlobalPtr<vir::Window>::instance();
                glm::ivec2 resolution = {window->width(), window->height()};
                setResolution(resolution, false);
            }
        }
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text(
R"(If checked, the layer aspect ratio is 
locked to that of the main window)"
            );
            ImGui::EndTooltip();
        }
        ImGui::SameLine();
        auto checkboxSize = ImGui::GetCursorPos().x-x0;
        ImGui::PushItemWidth(entryWidth-checkboxSize);
        glm::ivec2 resolution = resolution_;
        std::sprintf(label.get(), "##layer%dResolution", id_);
        if (ImGui::InputInt2(label.get(), glm::value_ptr(resolution)))
            setResolution(resolution, false, true);
        if (rendering_.target == Rendering::Target::Window)
            ImGui::EndDisabled();
        ImGui::PopItemWidth();
    
        if (rendering_.target != Rendering::Target::Window)
        {
            ImGui::SeparatorText("Framebuffer settings");
            {
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
                std::string selectedExportClearPolicy = "";
                static std::map
                <
                    Rendering::FramebufferClearPolicy, 
                    std::string
                > frameBufferClearPolicyToName
                {
                    {
                        Rendering::FramebufferClearPolicy::
                        ClearOnEveryFrameExport, 
                        "On every frame"
                    },
                    {
                        Rendering::FramebufferClearPolicy::
                        ClearOnFirstFrameExport, 
                        "On first frame"
                    },
                    {Rendering::FramebufferClearPolicy::None, "None"}
                };
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
                    selectedExportClearPolicy = 
                        frameBufferClearPolicyToName.at(rendering_.clearPolicy);
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

                ImGui::Text("Export clear policy  ");
                ImGui::SameLine();
                ImGui::PushItemWidth(entryWidth);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##layerExportClearPolicy",
                        selectedExportClearPolicy.c_str()
                    ) && rendering_.framebuffer != nullptr
                )
                {
                    for (auto entry : frameBufferClearPolicyToName)
                    {
                        if (ImGui::Selectable(entry.second.c_str()))
                            rendering_.clearPolicy = entry.first;
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
            }

            /*
            ImGui::SeparatorText("Post-processing effects");
            int iDelete = -1;
            static int iActive = -1;
            for (int i = 0; i < postProcesses_.size(); i++)
            {
                PostProcess* postProcess = postProcesses_[i];
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
                            postProcesses_[i] = postProcesses_[iActive];
                            postProcesses_[iActive] = postProcess;
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
                    {
                        *(postProcess->isGuiOpenPtr()) = true;
                        postProcess->renderGui();
                    }
                    ImGui::EndMenu();
                }
                else
                    *(postProcess->isGuiOpenPtr()) = false;
                ImGui::PopID();
            }
            if (iDelete != -1)
            {
                auto* postProcess = postProcesses_[iDelete];
                delete postProcess;
                postProcess = nullptr;
                postProcesses_.erase(postProcesses_.begin()+iDelete);
            }
            */
            
            // Selector for adding a new post-processing effect with the constraint
            // that each layer may have at most one post-processing effect of each
            // type
            /*
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
                    allAvailableTypes.reserve(vir::PostProcess::typeToName.size());
                    for (auto kv : vir::PostProcess::typeToName)
                        allAvailableTypes.push_back(kv.first);
                }
                std::vector<vir::PostProcess::Type> 
                    availableTypes(allAvailableTypes);
                for (auto* postProcess : postProcesses_)
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
                            PostProcess::create(app_, this, type);
                        if (postProcess != nullptr)
                            postProcesses_.emplace_back(postProcess);
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            */
        }
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderLayersTabBarGUI // Static
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
        float time = vir::GlobalPtr<vir::Time>::instance()->outerTime();
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
        const auto& headerErrors(layer->gui_.headerErrors);
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
        for (int i = 0; i < layers.size(); i++)
        {
            bool open = true;
            auto layer = layers[i];
            if(ImGui::BeginTabItem(layer->gui_.name.c_str(), &open))
            {
                layer->renderTabBarGUI(sharedUnifoms, resources);
                ImGui::EndTabItem();
            }
            if (!open) // I.e., if 'x' is pressed to delete the tab
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
                Resource::removeFramebufferFromResources
                (
                    &(layer->rendering_.framebuffer),
                    resources
                );
                delete layer;
                --i;
                continue;
            }
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
                layer->gui_.name = layer->gui_.newName;
                layer->flags_.rename = false;
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
    // Uniform::renderUniformsGUI::renderSharedUniformsGUI eventually called by
    // renderTabBarGUI
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

void Layer::renderTabBarGUI
(
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
                gui_.sourceEditor.renderFindReplaceToolGUI();
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
            gui_.sourceEditor.renderGUI("##sourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            bool madeReplacements = 
                gui_.sharedSourceEditor.renderFindReplaceToolGUI();
            flags_.uncompiledChanges = 
                flags_.uncompiledChanges || madeReplacements;
            gui_.sharedSourceEditor.renderGUI("##sharedSourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            if 
            (
                Uniform::renderUniformsGUI
                (
                    sharedUnifoms, 
                    uniforms_, 
                    uncompiledUniforms_,
                    *rendering_.shader,
                    resources
                )
            )
                flags_.uncompiledChanges = true;
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }

    
}

}
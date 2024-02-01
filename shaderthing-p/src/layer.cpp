#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/shareduniforms.h"
#include "shaderthing-p/include/uniform.h"
#include "shaderthing-p/include/helpers.h"
#include "shaderthing-p/include/macros.h"

#include "vir/include/vir.h"

#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imgui/imgui_internal.h"

namespace ShaderThing
{

bool Layer::Flags::errorsInSharedSource = false;

ImGuiExtd::TextEditor Layer::GUI::sharedSourceEditor = ImGuiExtd::TextEditor();

//----------------------------------------------------------------------------//

Layer::Layer
(
    const std::vector<Layer*>& layers,
    const SharedUniforms& sharedUniforms
) :
    id_(findFreeId(layers))
{
    depth_ = 1e-3*layers.size();

    // Set layer resolution from current app window resolution
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    resolution_ = {window->width(), window->height()};
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
    gui_.sourceEditor.SetText
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
    gui_.sourceEditor.ResetTextChanged();

    // Compile shader
    compileShader(sharedUniforms);

    // Init rendering quad
    glm::vec2 viewport =
    {
        aspectRatio_ > 1.f ? 1.f : aspectRatio_,
        aspectRatio_ < 1.f ? 1.0 : 1.0/aspectRatio_
    };
    rendering_.quad = new vir::Quad(viewport.x, viewport.y, depth_);

    // Register with event broadcaster
    this->tuneIn();
    this->receiverPriority() = -id_;
}

//----------------------------------------------------------------------------//

Layer::~Layer()
{
    DELETE_IF_NULLPTR(rendering_.framebufferA)
    DELETE_IF_NULLPTR(rendering_.framebufferB)
    DELETE_IF_NULLPTR(rendering_.shader)
    DELETE_IF_NULLPTR(rendering_.quad)
}

//----------------------------------------------------------------------------//

void Layer::onReceive(vir::Event::WindowResizeEvent& event)
{
    resolution_ = {event.width, event.height};
    aspectRatio_ = ((float)resolution_.x)/resolution_.y;
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

bool Layer::compileShader(const SharedUniforms& sharedUniforms)
{
    vir::Shader* shader = nullptr;
    std::exception_ptr exceptionPtr;
    auto headerAndLineCount = 
        fragmentShaderHeaderSourceAndLineCount(sharedUniforms);
    gui_.sourceHeader = std::get<std::string>(headerAndLineCount);
    unsigned int nHeaderLines = std::get<unsigned int>(headerAndLineCount);
    unsigned int nSharedLines = GUI::sharedSourceEditor.GetTotalLines();
    shader = vir::Shader::create
    (
        vertexShaderSource(sharedUniforms),
        (   // Full fragment source assembled on spot
            gui_.sourceHeader +
            gui_.sharedSourceEditor.GetText()+
            gui_.sourceEditor.GetText()
        ),
        vir::Shader::ConstructFrom::String,
        &exceptionPtr
    );
    flags_.errorsInSourceHeader = false;
    flags_.errorsInSharedSource = false;
    if (shader != nullptr) // Compilation success
    {
        delete rendering_.shader;
        rendering_.shader = shader;
        gui_.sharedSourceEditor.SetErrorMarkers({});
        gui_.sourceEditor.SetErrorMarkers({});
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
        flags_.uncompiledChanges = false;
        sharedUniforms.bindShader(rendering_.shader);
        // if (sharedUniforms.flags.resetFrameOnCompilation)
        // {
        //     sharedUniforms.cpuBlock.iFrame = 0;
        //     sharedUniforms.cpuBlock.iRenderPass = 0;
        // }
        return true;
    }
    try {std::rethrow_exception(exceptionPtr);}
    catch(std::exception& e)
    {
        // If the compilation fails, parse the error and show it
        // on the editor
        // flags.sourceEditsCompiled = true;
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
                errorIndex = 
                    std::stoi(errorIndexString)-nHeaderLines-nSharedLines;
                sharedError = false;
                if (errorIndex < 1)
                {
                    if (errorIndex > -nSharedLines)
                    {
                        sharedError = true;
                        flags_.errorsInSharedSource = true;
                        errorIndex += nSharedLines;
                    }
                    else
                    {
                        flags_.errorsInSourceHeader = true;
                        errorIndex = -1;
                    }
                }
                errorIndexString = "";
                if (firstErrorIndex == -1)
                    firstErrorIndex = errorIndex;
                readErrorIndex = false;
                while (exception[++i] == ' ' || exception[i] == ':'){}
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
            gui_.sourceEditor.SetErrorMarkers(errors);
            auto pos = ImGuiExtd::TextEditor::Coordinates(firstErrorIndex,0);
            gui_.sourceEditor.SetCursorPosition(pos);
        }
        if (sharedErrors.size() > 0)
        {
            gui_.sharedSourceEditor.SetErrorMarkers(sharedErrors);
        }
        return false;
    }
}

//----------------------------------------------------------------------------//

void Layer::renderShader
(
    vir::Framebuffer* target,
    const bool clearTarget,
    const SharedUniforms& sharedUniforms
)
{
    /*// Check if shader needs to be compiled
    if (layer->rendering.compileShader)
    {
        compileLayerShader(layer);
        layer->rendering.compileShader = false;
    }*/

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
                vir::Shader::ConstructFrom::String
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

void Layer::renderSettingsMenuGUI()
{
    if (ImGui::BeginMenu(("Layer ["+gui_.name+"]").c_str()))
    {
        ImGui::Text("Name ");
        ImGui::SameLine();
        static std::unique_ptr<char[]> label(new char[24]);
        std::sprintf(label.get(), "##layer%dInputText", id_);
        if (ImGui::InputText(label.get(), &gui_.newName))
            flags_.rename = true;
        ImGui::EndMenu();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderLayersTabBarGUI
(
    std::vector<Layer*>& layers,
    const SharedUniforms& sharedUnifoms
)
{
    // Logic for compilation button and, possibly, summary of compilation errors  
    static ImVec4 redColor = {1,0,0,1};
    static bool atLeastOneCompilationError(false);
    static bool atLeasOneUncompiledChange(false);
    if (atLeasOneUncompiledChange || atLeastOneCompilationError)
    {
        float time = vir::GlobalPtr<vir::Time>::instance()->outerTime();
        static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
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
        ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
        ImGui::Text("Ctrl+B");
        ImGui::PopStyleColor();
    }
    bool errorColorPushed = false;
    if (atLeastOneCompilationError)
    {
        ImGui::Separator();
        ImGui::PushStyleColor(ImGuiCol_Text, redColor);
        errorColorPushed = true;
        ImGui::Text("Compilation errors in:");
    }
    atLeastOneCompilationError = false;
    atLeasOneUncompiledChange = false;
    auto& sharedCompilationErrors
    (
        Layer::GUI::sharedSourceEditor.GetErrorMarkers()
    );
    if (sharedCompilationErrors.size() > 0)
    {
        if (!atLeastOneCompilationError)
            atLeastOneCompilationError = true;
        ImGui::Bullet();ImGui::Text("Common");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            for (auto& error : sharedCompilationErrors)
            {
                // First is line no., second is actual error text
                std::string errorText = 
                    "Line "+std::to_string(error.first)+": "+error.second;
                ImGui::Text(errorText.c_str());
            }
            ImGui::EndTooltip();
        }
    }
    for (auto* layer : layers)
    {
        auto& compilationErrors(layer->gui_.sourceEditor.GetErrorMarkers());
        if (compilationErrors.size() > 0 || layer->flags_.errorsInSourceHeader)
        {
            if (!atLeastOneCompilationError)
                atLeastOneCompilationError = true;
            ImGui::Bullet();ImGui::Text(layer->gui_.name.c_str());
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                if (layer->flags_.errorsInSourceHeader)
                {
                    std::string errorText =  
"Header: invalid uniform declaration(s), edit uniform name(s)";
                    ImGui::Text(errorText.c_str());
                }
                for (auto& error : compilationErrors)
                {
                    // First is line no., second is actual error text
                    std::string errorText = 
                        "Line "+std::to_string(error.first)+": "+error.second;
                    ImGui::Text(errorText.c_str());
                }
                ImGui::EndTooltip();
            }
        }
        if (layer->gui_.sourceEditor.IsTextChanged() || Layer::GUI::sharedSourceEditor.IsTextChanged())
            layer->flags_.uncompiledChanges = true;
        if (Layer::GUI::sharedSourceEditor.IsTextChanged())
            atLeasOneUncompiledChange = true;
        if (layer->flags_.uncompiledChanges && !atLeasOneUncompiledChange)
            atLeasOneUncompiledChange = true;
    }
    if (atLeastOneCompilationError)
        ImGui::Separator();
    if (errorColorPushed)
        ImGui::PopStyleColor();

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
                layer->renderTabBarGUI();
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
            auto tmp = layers[swap.first];
            layers[swap.first] = layers[swap.second];
            layers[swap.second] = tmp;
            swap = {0, 0};
        }
        ImGui::EndTabBar();
    }
}

//----------------------------------------------------------------------------//

void Layer::renderTabBarGUI()
{
    if (ImGui::BeginTabBar("##layerTabBar"))
    {
        if (ImGui::BeginTabItem("Fragment source"))
        {
            static ImVec4 redColor = {1,0,0,1};
            static ImVec4 grayColor = 
                ImGui::GetStyle().Colors[ImGuiCol_TextDisabled];
            static ImVec4 defaultColor = 
                ImGui::GetStyle().Colors[ImGuiCol_Text];
            //app_.findReplaceTextToolRef().renderGui();
            //flags_.uncompiledChanges =
            //    app_.findReplaceTextToolRef().findReplaceTextInEditor
            //    (
            //        fragmentSourceEditor_
            //    ) || hasUncompiledChanges_;
            //if (app_.findReplaceTextToolRef().isGuiOpen())
            //    ImGui::Separator();
            ImGui::Indent();
            if (flags_.errorsInSourceHeader)
                ImGui::PushStyleColor(ImGuiCol_Text, redColor);
            if (ImGui::TreeNode("Header"))
            {
                ImGui::PushStyleColor(ImGuiCol_Text, grayColor);
                ImGui::Text(gui_.sourceHeader.c_str());
                ImGui::PopStyleColor(); 
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::PushTextWrapPos(40.0f*ImGui::GetFontSize());
                    if (flags_.errorsInSourceHeader)
                    {
                        std::string error = 
"Header has error(s), likely due to invalid uniform declaration(s), correct "
"uniform name(s)";
                        ImGui::PushStyleColor(ImGuiCol_Text, redColor);
                        ImGui::Text(error.c_str());
                        ImGui::PopStyleColor();
                        ImGui::Separator();
                    }
                    ImGui::PushStyleColor(ImGuiCol_Text, defaultColor);
                    ImGui::Text("Header information:");
                    ImGui::Bullet();ImGui::Text(
"the highest possible GLSL version (based on your hardware) is used;");
                    ImGui::Bullet();ImGui::Text(
"the quad coordinate 'qc' varies in the [-.5, .5] range across the (current) "
"shortest side of the window, and from [-x/2, x/2] across its longest side, "
"wherein 'x' is the ratio between the lengths of the longest and the shortest "
"window sides. In practice, any line between e.g., two points described via  "
"this coordinate will maintain its angle if the window aspect ratio is "
"changed by resizing. The origin is at the window center;");
                    ImGui::Bullet();ImGui::Text(
"the texture coordinate 'tc' varies in the [0., 1.] range across both sides of "
"the window, regardless of the window size. The origin is at the bottom-left "
"corner of the window;");
                    ImGui::Bullet();ImGui::Text(
"uniform declarations are added automatically (on shader compilation) based "
"on the uniforms you specify in this layer's 'Uniforms' tab.");
                    ImGui::PopTextWrapPos();
                    ImGui::PopStyleColor();
                    ImGui::EndTooltip();
                }
                ImGui::TreePop();
                ImGui::Separator();
                ImGui::Dummy(ImVec2(-1, ImGui::GetTextLineHeight()));
            }
            if (flags_.errorsInSourceHeader)
                ImGui::PopStyleColor();
            ImGui::Unindent();
            gui_.sourceEditor.Render("##sourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Shared source"))
        {
            gui_.sharedSourceEditor.Render("##sharedSourceEditor");
            ImGui::EndTabItem();
        }
        if (ImGui::BeginTabItem("Uniforms"))
        {
            ImGui::EndTabItem();
        }
        ImGui::EndTabBar();
    }
}

}
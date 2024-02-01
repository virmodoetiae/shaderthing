#include "shaderthing-p/include/modules/backend.h"
#include "shaderthing-p/include/app.h"
#include "shaderthing-p/include/data/layer.h"
#include "shaderthing-p/include/data/uniform.h"
#include "shaderthing-p/include/data/shareduniforms.h"

#include "vir/include/vir.h"

#include "thirdparty/glm/glm.hpp"
#include "shaderthing-p/include/modules/backend.h"

namespace ShaderThing
{

namespace Backend
{

void initializeSharedUniforms(SharedUniforms& sharedUniforms)
{
    // Init CPU block dataW
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    sharedUniforms.cpuBlock.iResolution = 
        glm::ivec2{window->width(), window->height()};
    sharedUniforms.cpuBlock.iAspectRatio = 
        float(window->width())/float(window->height());
    for (int i=0; i<256; i++)
    {
        sharedUniforms.cpuBlock.iKeyboard[i] = glm::ivec3({0,0,0});
    }

    // Init cameras
    if (sharedUniforms.screenCamera == nullptr)
        sharedUniforms.screenCamera = vir::Camera::create<vir::Camera>();
    if (sharedUniforms.shaderCamera == nullptr)
        sharedUniforms.shaderCamera = vir::Camera::create<vir::InputCamera>();
    sharedUniforms.screenCamera->setProjectionType
    (
        vir::Camera::ProjectionType::Orthographic
    );
    sharedUniforms.screenCamera->setViewportHeight
    (
        std::min(1.0f, 1.0f/sharedUniforms.cpuBlock.iAspectRatio)
    );
    sharedUniforms.screenCamera->setPosition({0, 0, 1});
    sharedUniforms.shaderCamera->setZPlusIsLookDirection(true);
    sharedUniforms.shaderCamera->setDirection
    (
        sharedUniforms.cpuBlock.iLook.packed()
    );
    sharedUniforms.shaderCamera->setPosition
    (
        sharedUniforms.cpuBlock.iWASD.packed()
    );
    sharedUniforms.screenCamera->update();
    sharedUniforms.shaderCamera->update();
    sharedUniforms.cpuBlock.iMVP = 
        sharedUniforms.screenCamera->projectionViewMatrix();

    // Init GPU-side uniform block, bind and copy data from CPU
    if (sharedUniforms.gpuBlock == nullptr);
        sharedUniforms.gpuBlock = 
            vir::UniformBuffer::create(SharedUniforms::CPUBlock::size());
    sharedUniforms.gpuBlock->bind(sharedUniforms.gpuBindingPoint);
    sharedUniforms.gpuBlock->setData
    (
        &(sharedUniforms.cpuBlock),
        SharedUniforms::CPUBlock::size(),
        0
    );
}

void initialize(App& app)
{
    initializeSharedUniforms(app.sharedUniforms);
    createLayerIn(app.layers, app.sharedUniforms);
}

Layer* createLayerIn
(
    std::vector<Layer*>& layers,
    SharedUniforms& sharedUniforms
)
{
    // Create new layer with smallest available free id
    auto findFreeId = [](const std::vector<Layer*>& layers)
    {
        std::vector<unsigned int> ids(layers.size());
        unsigned int id(0);
        for (auto l : layers)
            ids[id++] = l->id;
        std::sort(ids.begin(), ids.end());
        for (id=0; id<layers.size(); id++)
        {
            if (id < ids[id])
                return id;
        }
        return id;
    };
    auto layer = new Layer{findFreeId(layers)};

    // Set layer detph
    layer->depth = 1e-3*layers.size();

    // Set layer resolution from current app window resolution
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    layer->resolution = {window->width(), window->height()};
    layer->aspectRatio = ((float)layer->resolution.x)/layer->resolution.y;

    // Add default uniforms
    auto addUniformsTo = [](Layer* layer)
    {
        Uniform* u = nullptr;
        
        u = new Uniform();
        u->specialType = Uniform::SpecialType::LayerAspectRatio;
        u->name = "iAspectRatio";
        u->type = Uniform::Type::Float;
        u->setValuePtr(&layer->aspectRatio);
        u->gui.showBounds = false;
        layer->uniforms.emplace_back(u);

        u = new Uniform();
        u->specialType = Uniform::SpecialType::LayerResolution;
        u->name = "iResolution";
        u->type = Uniform::Type::Float2;
        u->setValuePtr(&layer->resolution);
        u->gui.bounds = glm::vec2(1.0f, 4096.0f);
        u->gui.showBounds = false;
        layer->uniforms.emplace_back(u);
    };
    addUniformsTo(layer);

    // Set name
    layer->gui.name = "Layer "+std::to_string(layer->id);
    layer->gui.newName = layer->gui.name;

    // Set default fragment source in editor
    layer->gui.sourceEditor.SetText
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

    // Compile shader
    compileLayerShader(layer, sharedUniforms);

    // Init rendering quad
    glm::vec2 viewport =
    {
        layer->aspectRatio > 1.f ? 1.f : layer->aspectRatio,
        layer->aspectRatio < 1.f ? 1.0 : 1.0/layer->aspectRatio
    };
    layer->rendering.quad = new vir::Quad(viewport.x, viewport.y, layer->depth);

    // Add to list & return
    layers.emplace_back(layer);

    return layer;
}

//----------------------------------------------------------------------------//

void deleteLayerFrom(Layer* layer, std::vector<Layer*>& layers)
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
}

//----------------------------------------------------------------------------//

void constrainAndSetWindowResolution(glm::ivec2& resolution)
{
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    auto monitorScale = window->contentScale();
    glm::ivec2 minResolution = {120*monitorScale.x, 1};
    glm::ivec2 maxResolution = window->primaryMonitorResolution();
    resolution.x = 
        std::max(std::min(resolution.x, maxResolution.x), minResolution.x);
    resolution.y = 
        std::max(std::min(resolution.y, maxResolution.y), minResolution.y);
    if (window->width() != resolution.x || window->height() != resolution.y)
        window->setSize
        (
            resolution.x,
            resolution.y
        );
}

//----------------------------------------------------------------------------//

const std::string& shaderVersionSource()
{
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    static const std::string version
    (
        "#version "+window->context()->shadingLanguageVersion()+" core\n"
    );
    return version;
}

//----------------------------------------------------------------------------//

const std::string& vertexShaderSource()
{
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    static const std::string vertexSource
    (
        shaderVersionSource()+
R"(layout (location=0) in vec3 iqc;
layout (location=1) in vec2 itc;
out vec2 qc;
out vec2 tc;
)" + SharedUniforms::glslBlockSource +
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

std::string assembleFragmentShaderSource
(
    Layer* layer, 
    unsigned int& nHeaderLines
)
{
    nHeaderLines = 16;
    static std::string header
    (
        shaderVersionSource()+
        "in vec2 qc;\nin vec2 tc;\nout vec4 fragColor;\n"+
        SharedUniforms::glslBlockSource
    );
    layer->gui.sourceHeader = header;

    for (auto* u : layer->uniforms)
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
        layer->gui.sourceHeader += 
            "uniform "+typeName+" "+u->name+";\n";
        ++nHeaderLines;
        // Automatically managed sampler2D resolution uniform
        if (isSampler2D)
        {
            layer->gui.sourceHeader +=
                "uniform vec2 "+u->name+"Resolution;\n";
            ++nHeaderLines;
        }
    }
    int nSharedLines = layer->gui.sharedSourceEditor.GetTotalLines();
    return 
        layer->gui.sourceHeader +
        layer->gui.sharedSourceEditor.GetText()+"\n"+
        layer->gui.sourceEditor.GetText();
}

//----------------------------------------------------------------------------//

bool compileLayerShader(Layer* layer, SharedUniforms& sharedUniforms)
{
    //if (layer->gui.sourceEditsCompiled)
    //    return true;

    vir::Shader* shader = nullptr;
    std::exception_ptr exceptionPtr;
    unsigned int nHeaderLines = 0;
    shader = vir::Shader::create
    (
        vertexShaderSource(),
        assembleFragmentShaderSource
        (
            layer,
            nHeaderLines
        ), 
        vir::Shader::ConstructFrom::String,
        &exceptionPtr
    );
    layer->gui.errorsInSourceHeader = false;
    layer->gui.errorsInSharedSource = false;
    if (shader != nullptr) // Compilation success
    {
        delete layer->rendering.shader;
        layer->rendering.shader = shader;
        layer->gui.sharedSourceEditor.SetErrorMarkers({});
        layer->gui.sourceEditor.SetErrorMarkers({});
        // Remove only named uniforms form the list of the uncompiled uniforms
        // as they are the only ones that get compiled into the source code
        layer->uncompiledUniforms.erase
        (
            std::remove_if
            (
                layer->uncompiledUniforms.begin(),
                layer->uncompiledUniforms.end(),
                [](Uniform* u){return u->name.size()>0;}
            ),
            layer->uncompiledUniforms.end()
        );
        layer->gui.sourceEditsCompiled = false;
        layer->rendering.shader->bindUniformBlock
        (
            sharedUniforms.glslBlockName,
            *sharedUniforms.gpuBlock,
            sharedUniforms.gpuBindingPoint
        );
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
        layer->gui.sourceEditsCompiled = true;
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
                        layer->gui.sharedSourceEditor.GetTotalLines();
                    if (errorIndex > -nSharedLines)
                    {
                        sharedError = true;
                        layer->gui.errorsInSharedSource = true;
                        errorIndex += nSharedLines;
                    }
                    else
                    {
                        layer->gui.errorsInSourceHeader = true;
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
            layer->gui.sourceEditor.SetErrorMarkers(errors);
            auto pos = ImGuiExtd::TextEditor::Coordinates(firstErrorIndex,0);
            layer->gui.sourceEditor.SetCursorPosition(pos);
        }
        if (sharedErrors.size() > 0)
        {
            layer->gui.sharedSourceEditor.SetErrorMarkers(sharedErrors);
        }
        return false;
    }
}

//----------------------------------------------------------------------------//

void renderLayerShader
(
    Layer* layer,
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
    layer->rendering.flippedBuffers = !layer->rendering.flippedBuffers;
    layer->rendering.framebuffer = 
        layer->rendering.flippedBuffers ? 
        layer->rendering.framebufferA :
        layer->rendering.framebufferB;

    // Set all uniforms that are not set in the GUI step
    // setSharedDefaultSamplerUniforms();
    

    // Re-direct rendering & disable blending if not rendering to the window
    static auto renderer = vir::GlobalPtr<vir::Renderer>::instance();
    bool blendingEnabled = true;
    vir::Framebuffer* target0(target);
    if (layer->rendering.target != Layer::Rendering::Target::Window)
    {
        target = layer->rendering.framebuffer;
        renderer->setBlending(false);
        blendingEnabled = false;
    }

    // Actual render call
    layer->rendering.shader->bind();
    layer->rendering.shader->setUniformMat4("iMVP2", sharedUniforms.cpuBlock.iMVP);
    renderer->submit
    (
        *layer->rendering.quad,
        layer->rendering.shader,
        target,
        clearTarget || // Or force clear if not rendering to window
        layer->rendering.target != Layer::Rendering::Target::Window
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
        layer->rendering.target != 
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
                vertexShaderSource(),
                shaderVersionSource()+
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
    layer->rendering.framebuffer->bindColorBuffer(0);
    internalFramebufferShader->setUniformInt("self", 0);
    renderer->submit
    (
        *layer->rendering.quad, 
        internalFramebufferShader.get(), 
        target0,
        clearTarget
    );
}

void update(App& app)
{
    static const auto window = vir::GlobalPtr<vir::Window>::instance();
    app.sharedUniforms.cpuBlock.iTime += window->time()->outerTimestep();
    app.sharedUniforms.cpuBlock.iFrame += 1;

    app.sharedUniforms.gpuBlock->setData
    (
        &app.sharedUniforms.cpuBlock,
        app.sharedUniforms.cpuBlock.size(),
        0
    );
}

}

}
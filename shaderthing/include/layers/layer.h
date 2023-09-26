#ifndef ST_LAYER_H
#define ST_LAYER_H

#include <vector>
#include <string>
#include <unordered_map>

#include "vir/include/vir.h"
#include "thirdparty/imguitexteditor/imguitexteditor.h"

//----------------------------------------------------------------------------//

namespace ShaderThing
{

class ShaderThingApp;
class Resource;

class Layer
{
public:

    enum class RendersTo
    {
        Window,
        InternalFramebuffer,
        InternalFramebufferAndWindow
    };
    static RendersTo rendererTargets[3];
    static std::string rendererTargetNames[3];
    static std::unordered_map<RendersTo, std::string> rendererTargetToName;
    static std::unordered_map<std::string, RendersTo> nameToRendererTarget;
    static std::unordered_map<vir::TextureBuffer::WrapMode, std::string> 
        wrapModeToName;
    static std::string supportedUniformTypeNames[11];
    static vir::TextureBuffer::InternalFormat supportedInternalFormats[2];

private :

    // Default shader sources
    static std::string defaultVertexSource_;
    static std::string defaultFragmentSource_;

    // Transparent shader rendered to screen when the buffer is rendering to
    // framebuffers to avoid visual glitches
    static vir::Shader* voidShader_;

    // Shader for rendering the internal framebuffer
    static vir::Shader* internalFramebufferShader_;

    // Ref to top level app
    ShaderThingApp& app_;
    
    // Id of the buffer
    uint32_t id_;

    // 
    RendersTo rendersTo_;

    // State flags
    bool toBeDeleted_;
    bool toBeRenamed_;
    bool toBeCompiled_;
    bool isGuiRendered_;
    bool isGuiDeletionConfirmationPending_;

    // Z coordinate of the screen quad associated with this buffer
    float depth_;

    // Buffer resolution
    glm::ivec2 resolution_;

    // Target resize resolutions (buffered for delayed resize)
    glm::ivec2 targetResolution_;

    //
    float resolutionScale_;

    // Viewport dimensions
    glm::vec2 viewport_;

    // Name of the buffer (i.e. of the ImGui tab)
    std::string name_;

    // A temporary placeholder to allow for live buffer name editing
    std::string targetName_;

    // Actual source code (as string) of the fragment shader and editor
    std::string fragmentSource_;
    ImGuiExtd::TextEditor fragmentEditor_;
    bool uncompiledFragmentEditorChanges_;
    
    // Screen quad associated with this buffer which is used a canvas for the
    // shader
    vir::Quad* screenQuad_;

    // Framebuffers for off-screen rendering (when rendersTo_ == Self)
    bool flipFramebuffers_;
    vir::Framebuffer* writeOnlyFramebuffer_;
    vir::Framebuffer* readOnlyFramebuffer_;
    vir::Framebuffer* framebufferA_;
    vir::Framebuffer* framebufferB_;

    // Shader
    vir::Shader* shader_;
    int shaderId0_;

    // List of user-editable uniforms associated with the shader
    std::vector<vir::Shader::Uniform*> uniforms_;
    
    //
    std::unordered_map<vir::Shader::Uniform*, glm::vec2> uniformLimits_;

    // List of uniforms provided by default (e.g., camera, time, etc.)
    std::vector<vir::Shader::Uniform*> defaultUniforms_;

    // Ref to global time and frame
    float& time_;
    bool& timePaused_;
    int& frame_;

    // Ref to global camera for looking at the quad
    vir::Camera& screenCamera_;

    // Ref to global in-shader camera (might as well be shader-specific, for
    // now it is fine like this)
    vir::Camera& shaderCamera_;

    // Ref to global renderer object
    vir::Renderer& renderer_;

    // Cached old default uniform values to avoid always setting them
    glm::mat4 mvp0_;
    glm::ivec2 resolution0_;
    glm::vec3 cameraPosition0_;
    glm::vec3 cameraDirection0_;
    glm::ivec4 mouse0_;

    //
    std::unordered_map<vir::Shader::Uniform*, bool> uniformUsesColorPicker_;

    // If a another layer was among the list of sampler2D-type uniforms to
    // this layer, there is not guarantee that the latter layer will be
    // loaded before this layer on project loading. Thus, their names are
    // cached and used for uniform re-setting after all layers have been
    // loaded (done in resetPostLoadLayerUniforms())
    std::unordered_map<vir::Shader::Uniform*, std::string>
        uniformLayerNamesToBeSet_;

    // These functions are responsible for generating the full GLSL source
    // code by adding required #version/in/out/uniform lines to the headers of
    // the provided source files
    static const std::string& assembleVertexSource();
    std::string assembleFragmentSource
    (
        const std::string& source, 
        int* nHeaderLines=nullptr
    );
    
    void createStaticShaders();
    void compileShader();
    void initializeDefaultUniforms();
    void setDefaultAndSamplerUniforms();
    void setNonDefaultUniforms();
    void adjustTargetResolution();
    void rebuildFramebuffers
    (
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    );
    void renderGuiMain();
    void renderGuiConfirmDeletion(); 
    void renderGuiUniforms();

public:

    Layer
    (
        ShaderThingApp& app,
        glm::ivec2 resolution,
        float depth
    );
    
    // Construct from serialized data on disk, i.e., equivalent to a loadState
    // function
    Layer
    (
        ShaderThingApp& app,
        std::string& source,
        uint32_t& index,
        bool isGuiRendered = false
    );

    ~Layer();

    //
    void clearFramebuffers();

    // Removes the given resorces from this buffer's uniforms list, if present
    void removeResourceFromUniforms(Resource* resource);

    //
    void reBindLayerUniforms();

    // Render the shader to the provided framebuffer (or to the window if no
    // framebuffer provided)
    void render
    (
        vir::Framebuffer* exportFramebuffer = nullptr, 
        bool clearTarget=true
    );
    
    // Render this buffer's internal framebuffer texture to the provided target
    void renderInternalFramebuffer
    (
        vir::Framebuffer* exportFramebuffer = nullptr, 
        bool clearTarget=true
    );

    // GUI-related (all implemented in buffer_gui.cpp)
    void renderGuiSettings();
    void renderGui();

    // Updates viewport, framebuffer sizes and name
    void update();

    // Serialize buffer data
    void saveState(std::ofstream&);
    
    // Accessors
    uint32_t id() const {return id_;}
    RendersTo rendersTo() const {return rendersTo_;}
    bool toBeDeleted() const {return toBeDeleted_;}
    bool toBeRenamed() const {return toBeRenamed_;}
    bool toBeCompiled() const {return toBeCompiled_;}
    glm::vec2 resolution() const {return resolution_;}
    float resolutionScale() const {return resolutionScale_;}
    float depth() const {return depth_;}
    std::string name() const {return name_;}
    std::string& nameRef() {return name_;}
    vir::Framebuffer*& readOnlyFramebuffer() 
    {
        // Should be readOnlyFramebuffer_, right? Well, yes, but I am
        // experimenting with a different approach (yes, I am still double-
        // buffering and no, it hasn't become useless because of this)
        return writeOnlyFramebuffer_;
    }
    vir::Framebuffer*& writeOnlyFramebuffer() {return writeOnlyFramebuffer_;}

    // Setters
    void setDepth(float);
    void setName(std::string);
    void setTargetResolution(glm::ivec2, bool rescale=true);

    // Operators
    bool operator==(const Layer& rhs) {return id_ == rhs.id();}
};

}

#endif
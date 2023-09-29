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

// Enums ---------------------------------------------------------------------//

public :
    enum class RendersTo
    {
        Window,
        InternalFramebuffer,
        InternalFramebufferAndWindow
    };

// Static members ------------------------------------------------------------//

private:
    
    // Mappings
    static std::unordered_map<RendersTo, std::string> renderTargetToName;
    static std::unordered_map<vir::TextureBuffer::WrapMode, std::string> 
        wrapModeToName;
    
    //
    static std::string supportedUniformTypeNames[11];
    static vir::TextureBuffer::InternalFormat supportedInternalFormats[2];
    
    // Default vertex (unmodifiable) and fragment shader source codes used
    // for newly created layers
    static std::string defaultVertexSource_;
    static std::string defaultFragmentSource_;
    
    // Transparent shader rendered to screen when the buffer is rendering to
    // framebuffers to avoid visual glitches
    static vir::Shader* voidShader_;

    // Shader for rendering the internal framebuffer
    static vir::Shader* internalFramebufferShader_;

    // Shared source code editor
    static ImGuiExtd::TextEditor sharedSourceEditor_;
    // True if the shared source code has compilation errors
    static bool sharedSourceHasErrors_;

    // This function is responsible for generating the full GLSL source vertex
    // code by adding required #version/in/out/uniform lines to the vertex
    // shared source. Since the vertex source is the same for all layers and
    // cannot be modified, this is static
    static const std::string& assembleVertexSource();

public:

    // Number of text lines in shared source code
    static int nSharedEditorLines() 
    {
        return Layer::sharedSourceEditor_.GetTotalLines();
    }
    // Get content of shared source code
    static std::string sharedSource()
    {
        return Layer::sharedSourceEditor_.GetText();
    }
    // Set content of shared source code
    static void setSharedSource(const std::string& code)
    {
        Layer::sharedSourceEditor_.SetText(code);
        Layer::sharedSourceEditor_.ResetTextChanged();
    }
    // True if there are any compilation errors due to the shared source code
    static bool sharedSourceHasErrors() {return Layer::sharedSourceHasErrors_;}
    // Get list of shared source code compilation error line numbers and 
    // actual errors (mapped by line number)
    static const std::map<int, std::string>& sharedCompilationErrors()
    {
        return sharedSourceEditor_.GetErrorMarkers();
    }

// Non-static members --------------------------------------------------------//

private :

    // Ref to top level app
    ShaderThingApp& app_;
    
    // Id of the buffer
    uint32_t id_;
    // Name of the buffer (i.e. of the ImGui tab)
    std::string name_;
    // Name placeholder for deferred updates
    std::string targetName_;

    // The entity to which this layer renders to (window or internal 
    // framebuffer)
    RendersTo rendersTo_;

    // State flags for deferred updates
    bool toBeDeleted_;
    bool toBeRenamed_;
    bool toBeCompiled_;
    bool isGuiRendered_;
    bool isGuiDeletionConfirmationPending_;
    bool hasUncompiledChanges_;
    // True if compilation errors due to fragmentSourceHeader_
    bool hasHeaderErrors_; 
    // Double buffering state flag used to determine which, between 
    // framebufferA_ and framebufferB_, is the one currently used for
    // writing and which one for reading
    bool flipFramebuffers_; 

    // Buffer resolution
    glm::ivec2 resolution_;
    // Target resize resolution for deferred update
    glm::ivec2 targetResolution_;
    // Ratio of this layer's resolution to the main window resolution. Since
    // the aspect ratio of the layer is tied to that of the window, this number
    // is a single float
    float resolutionScale_;
    // Viewport dimensions
    glm::vec2 viewport_;

    // Fragment shared header code (i.e., #version, in/out/uniform declarations)
    std::string fragmentSourceHeader_;
    // Fragment shared source code, always exclusive of shared code and
    // header code (i.e., #version, in/out/uniform declarations)
    std::string fragmentSource_;
    // Fragment shader text editor
    ImGuiExtd::TextEditor fragmentSourceEditor_;
    
    // Screen quad associated with this buffer which is used a canvas for the
    // shader
    vir::Quad* screenQuad_;
    // Z coordinate of the screen quad associated with this buffer
    float depth_;

    // Framebuffers (used when rendering to internal framebuffer). There is
    // actually more than one because double-buffering is used
    vir::Framebuffer* writeOnlyFramebuffer_;
    vir::Framebuffer* readOnlyFramebuffer_;
    vir::Framebuffer* framebufferA_;
    vir::Framebuffer* framebufferB_;

    // Shader
    vir::Shader* shader_;
    // Cached id of the previously compiled shared, used for checking shader
    // updates
    int shaderId0_;

    // List of user-editable uniforms associated with the shader (exlcusive of
    // defaultUniforms_!)
    std::vector<vir::Shader::Uniform*> uniforms_;
    // List of uniforms provided by default (e.g., camera, time, etc.)
    std::vector<vir::Shader::Uniform*> defaultUniforms_;
    // For each uniform, this map contains the minimum and maximum allowable
    // values stored in a vec2-style variable
    std::unordered_map<vir::Shader::Uniform*, glm::vec2> uniformLimits_;
    // List of all uniforms that have been added via the Uniforms tab GUI, but
    // not yet compiled. They are embedded in fragmentSourceHeader_ on
    // compilation
    std::vector<vir::Shader::Uniform*> uncompiledUniforms_;
    // List of uniforms which are to be modified via an ImGui color picker tool
    std::unordered_map<vir::Shader::Uniform*, bool> uniformUsesColorPicker_;
    // If another layer was among the list of sampler2D-type uniforms of
    // this layer, there is not guarantee that the latter layer will be
    // loaded before this layer on project loading. Thus, their names are
    // cached and used for uniform re-setting after all layers have been
    // loaded (done in rebindLayerUniforms())
    std::unordered_map<vir::Shader::Uniform*, std::string>
        uniformLayerNamesToBeSet_;

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

    // This function is responsible for generating the full GLSL source fragment
    // code by adding required #version/in/out/uniform lines as well as the 
    // shared source code to the provided source. It can optionally return the
    // total number of lines added to source via the nHeaderLines ptr
    std::string assembleFragmentSource
    (
        const std::string& source, 
        int* nHeaderLines=nullptr
    );
    
    // Create instances of voidShader_, internalFramebufferShader_, if not done
    // so already
    void createStaticShaders();
    
    // Set defaults for the fragmentSourceEditor_, sharedSourceEditor_
    void initializeEditors();

    // Initialize defaultUniforms_ list
    void initializeDefaultUniforms();

    // Bind uniform values to actual shader uniform before rendering. This
    // only binds uniforms in defaultUniforms_ and all uniforms in uniforms_
    // that are of Sampler2D or SamplerCube type
    void setDefaultAndSamplerUniforms();

    // Bind all uniforms (other than uniforms in defaultUniforms_ nor uniforms 
    // in uniforms_ of types Sampler2D or SamplerCube) to the actual shader 
    // uniforms before rendering
    void setNonDefaultUniforms();

    // Correct targetResolution_ before setting it (this does not set it) to
    // resolution_ in order to satisfy certain constraints
    void adjustTargetResolution();

    // Empty the contents of framebufferA_, framebufferB_, reconstruct them
    // from scratch with the provided internal data format specified and
    // resolution
    void rebuildFramebuffers
    (
        const vir::TextureBuffer::InternalFormat& internalFormat, 
        const glm::ivec2& resolution
    );

    // Generate the final GLSL source code from fragmentSourceHeader_,
    // sharedSourceEditor_ contents and fragmentSource_ and compiled the
    // resulting shader. If errors arise, set relevant stateFlags, and set
    // error markers in fragmentSourceEditor_ and/or sharedSourceEditor_
    void compileShader();

    // Functions to be called only from within an ImGui-environment

    // Render the main Layer view (i.e., the one which contains the 
    // "Fragment source" and "Uniforms" tabs)
    void renderGuiMain();

    // Render this Layer's deletion confirmation modal
    void renderGuiConfirmDeletion(); 

    // Render this Layer's Uniform's tab
    void renderGuiUniforms();

public:

    // Construct from top-level app reference, layer resolution and layer depth
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

    // Destructor
    ~Layer();

    // Clear the internal framebuffer (i.e., framebufferA_ and framebufferB_) of
    // their contents
    void clearFramebuffers();

    // Removes the given resorce from this layers's uniforms list, if found
    void removeResourceFromUniforms(Resource* resource);

    // If another layer was among the list of sampler2D-type uniforms of
    // this layer, there is not guarantee that the latter layer will be
    // loaded before this layer on project loading. Thus, their names are
    // cached and used for uniform rebinding after all layers have been
    // loaded. This rebinding is what this function is for
    void rebindLayerUniforms();

    // Render the shader to the provided framebuffer (or to the window if no
    // framebuffer provided). Optionally, clear the existing contents of the
    // render target before rendering (default clear color is transparent
    // black, i.e., RGBA={0,0,0,0})
    void render
    (
        vir::Framebuffer* exportFramebuffer = nullptr, 
        bool clearTarget=true
    );
    
    // Render this buffer's internal framebuffer color attachment to the 
    // provided target. Optionally, clear the existing contents of the
    // render target before rendering (default clear color is transparent
    // black, i.e., RGBA={0,0,0,0})
    void renderInternalFramebuffer
    (
        vir::Framebuffer* exportFramebuffer = nullptr, 
        bool clearTarget=true
    );

    // Render the GUI view when hovering over Settings->[This layer's name]
    void renderGuiSettings();

    // Render main view of this layer (i.e., the one which contains the 
    // "Fragment source" and "Uniforms" tabs). This is a wrapper of the
    // private renderGuiMain(), and integrates checks on buffer deletion
    void renderGui();

    // Update layer name, resolution, framebuffers, compilation status (i.e.,
    // are there any changes that require a re-compilation?). Most of the
    // deferred updates happen here
    void update();

    // Serialize buffer data
    void saveState(std::ofstream& stream);

    // Mark this layer's fragment shader for compilation
    void markForCompilation(){toBeCompiled_ = true;}
    
    // Accessors -------------------------------------------------------------//

    //
    uint32_t id() const {return id_;}
    
    //
    RendersTo rendersTo() const {return rendersTo_;}
    
    //
    bool toBeDeleted() const {return toBeDeleted_;}
    
    //
    bool toBeRenamed() const {return toBeRenamed_;}
    
    //
    bool toBeCompiled() const {return toBeCompiled_;}
    
    //
    glm::vec2 resolution() const {return resolution_;}
    
    //
    float resolutionScale() const {return resolutionScale_;}
    
    //
    float depth() const {return depth_;}
    
    //
    std::string name() const {return name_;}
    
    //
    std::string& nameRef() {return name_;}
    
    //
    bool hasHeaderErrors() const {return hasHeaderErrors_;}
    
    //
    const std::map<int, std::string>& compilationErrors() const
    {
        return fragmentSourceEditor_.GetErrorMarkers();
    }
    
    //
    bool hasUncompiledChanges() const {return hasUncompiledChanges_;}
    
    //
    vir::Framebuffer*& readOnlyFramebuffer() 
    {
        // Should be readOnlyFramebuffer_, right? Well, yes, but I am
        // experimenting with a different approach (yes, I am still double-
        // buffering and no, it hasn't become useless because of this)
        return writeOnlyFramebuffer_;
    }
    
    //
    vir::Framebuffer*& writeOnlyFramebuffer() {return writeOnlyFramebuffer_;}

    // Setters ---------------------------------------------------------------//

    //
    void setDepth(float);
    
    //
    void setName(std::string);
    
    //
    void setTargetResolution(glm::ivec2, bool rescale=true);

    // Operators -------------------------------------------------------------//
    
    bool operator==(const Layer& rhs) {return id_ == rhs.id();}

};

}

#endif
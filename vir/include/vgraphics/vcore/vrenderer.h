#ifndef V_RENDERER_H
#define V_RENDERER_H

namespace vir
{

class VertexArray;
class Shader;
class Framebuffer;
class GeometricPrimitive;

class RendererAPI
{
public:
    virtual ~RendererAPI(){}
    virtual void clear(float r=.0f, float g=.0f, float b=.0f, float a=.0f) = 0;
    virtual void drawIndexed(VertexArray&) = 0;
    virtual void setDepthTesting(bool) = 0;
    virtual void setFaceCulling(bool) = 0;
    virtual void setBlending(bool) = 0;
};

class Renderer
{
protected:

    // Name of the rendering device (i.e., the graphics card)
    std::string deviceName_;

    // Actual (low-level) api-specific rendering commands
    RendererAPI* api_;

    // Protected constructor
    Renderer(){}

    // Delete all other constructors
    Renderer(const Renderer&) = delete;
    Renderer& operator=(const Renderer& other) = delete;
    Renderer(Renderer&&) = delete;
    Renderer& operator=(Renderer&& other) = delete;

public:

    virtual ~Renderer();

    // Accessors
    const std::string& deviceName() const {return deviceName_;}

    // Use this to initialize the GlobalPtr to Renderer
    static Renderer* initialize();

    // Low-level accessors (just in case...)
    void clear(float r=.0f, float g=.0f, float b=.0f, float a=.0f) 
    {
        api_->clear(r,g,b,a);
    }
    void setDepthTesting(bool flag) {api_->setDepthTesting(flag);}
    void setFaceCulling(bool flag){api_->setFaceCulling(flag);}
    void setBlending(bool flag){api_->setBlending(flag);}
    
    // High-level functions
    virtual void beginScene() = 0;
    virtual void submit
    (
        VertexArray* vertexArray, 
        Shader* shader
    ) = 0;
    virtual void submit
    (
        GeometricPrimitive& geometricPrimitive, 
        Shader* shader, 
        Framebuffer* target = nullptr, 
        bool clearTarget = true
    );
    virtual void endScene() = 0;

    static Renderer* instance() {return GlobalPtr<Renderer>::instance();}
    
};

}

#endif
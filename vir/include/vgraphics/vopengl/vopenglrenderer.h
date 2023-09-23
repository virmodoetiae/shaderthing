#ifndef V_OPENGL_RENDERER_H
#define V_OPENGL_RENDERER_H

#include "vgraphics/vrenderer.h"

namespace vir
{

class OpenGLRendererAPI : public RendererAPI
{
public:
    void clear(float r=.0f, float g=.0f, float b=.0f, float a=.0f) override;
    void drawIndexed(VertexArray&) override;
    void setDepthTesting(bool) override;
    void setFaceCulling(bool) override;
    void setBlending(bool) override;
};

class OpenGLRenderer : public Renderer
{
protected:

    // Delete all other constructors
    OpenGLRenderer(const OpenGLRenderer&) = delete;
    OpenGLRenderer& operator=(const OpenGLRenderer& other) = delete;
    OpenGLRenderer(OpenGLRenderer&&) = delete;
    OpenGLRenderer& operator=(OpenGLRenderer&& other) = delete;

public:

    OpenGLRenderer();

    void beginScene() override;
    void endScene() override;
    void submit
    (
        VertexArray* vertexArray, 
        Shader* shader
    ) override;
};

}

#endif
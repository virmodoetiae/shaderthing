#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglrenderer.h"

namespace vir
{

// OpenGL API ----------------------------------------------------------------//

void OpenGLRendererAPI::clear(float r, float g, float b, float a)
{
    // Set the clear color
    glClearColor(r, g, b, a);
    // Clear the screen with the set clear color
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
}

void OpenGLRendererAPI::drawIndexed(VertexArray& vertexArray)
{
    glDrawElements
    (
        GL_TRIANGLES, 
        vertexArray.indexBuffer()->count(), 
        GL_UNSIGNED_INT, 
        0
    );
}

void OpenGLRendererAPI::setDepthTesting(bool flag)
{
    if (flag)
        glEnable(GL_DEPTH_TEST);
    else
        glDisable(GL_DEPTH_TEST);
}

void OpenGLRendererAPI::setFaceCulling(bool flag)
{
    if (flag)
        glEnable(GL_CULL_FACE);
    else
        glDisable(GL_CULL_FACE);
}

void OpenGLRendererAPI::setBlending(bool flag)
{
    if (flag)
    {
        glEnable(GL_BLEND);
        
        // If a pixel of color C0 already exists on a target buffer on which
        // rendering is to take place, and the rendering process determines
        // that a new pixel C1 is incoming to the location of C0, the final
        // pixel color will be (in GLSL-like notation):
        // C = vec4(C0.a*C0.rgb+(1-C0.a)*C1.rgb, 1*C0.a + (1-C0.a)*C1.a)
        glBlendFuncSeparate
        (
            GL_SRC_ALPHA, 
            GL_ONE_MINUS_SRC_ALPHA,
            GL_ONE,
            GL_ONE_MINUS_SRC_ALPHA
        );
    }
    else
        glDisable(GL_BLEND);
}

// Renderer ------------------------------------------------------------------//

OpenGLRenderer::OpenGLRenderer()
{
    api_ = new OpenGLRendererAPI();
    deviceName_ = reinterpret_cast<const char*>(glGetString(GL_RENDERER));
}

void OpenGLRenderer::beginScene()
{
    api_->clear();
}

void OpenGLRenderer::endScene()
{
    
}

void OpenGLRenderer::submit(VertexArray* vertexArray, Shader* shader)
{
    shader->bind();
    vertexArray->bind();
    api_->drawIndexed(*vertexArray);
}

}
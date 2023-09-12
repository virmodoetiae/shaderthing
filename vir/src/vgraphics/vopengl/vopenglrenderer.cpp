#include "vpch.h"
#include "vgraphics/vopengl/vopenglrenderer.h"

#if defined(__WIN32__)
#include "windows.h"
extern "C" {
    __declspec(dllexport) DWORD NvOptimusEnablement = 1;
    __declspec(dllexport) int AmdPowerXpressRequestHighPerformance = 1;
}
#endif

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
        // I am hardcoding the most commonly used blending function for 
        // simplicity. This one means that if a pixel already has a color C0
        // and I am writing a new color C1 to it, the final color will be
        // C1.a*C1.rgb+(1.0-C1.a)*C0.rgb. The incoming color is called source,
        // the already present one is called destination
        glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
    }
    else
        glDisable(GL_BLEND);
}

// Renderer ------------------------------------------------------------------//

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
#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglmisc.h"

namespace vir
{

void OpenGLWaitSync()
{
    auto dataSync = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
    while (dataSync)
    {
        GLenum wait = glClientWaitSync
        (
            dataSync, 
            GL_SYNC_FLUSH_COMMANDS_BIT, 
            1000000 // ns == 1000 us timeout
        );
        if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED)
        {
            glDeleteSync(dataSync);
            break;
        }
    }   
}

GLuint findFreeSSBOBindingPoint()
{
    // Find a free SSBO binding point
    GLint maxBindings = 0;
    glGetIntegerv(GL_MAX_SHADER_STORAGE_BUFFER_BINDINGS, &maxBindings);
    for (GLuint binding = 0; binding < (GLuint)maxBindings; ++binding)
    {
        GLint boundBuffer = 0;
        glGetIntegeri_v(GL_SHADER_STORAGE_BUFFER_BINDING, binding, &boundBuffer);
        if (boundBuffer == 0) 
            return binding;
    }
    return 0; // Not the best option
}

}
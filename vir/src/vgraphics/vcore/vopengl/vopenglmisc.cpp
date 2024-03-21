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

}
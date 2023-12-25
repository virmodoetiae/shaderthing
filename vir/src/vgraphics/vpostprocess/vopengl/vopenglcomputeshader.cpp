#include "vpch.h"
#include "vgraphics/vpostprocess/vopengl/vopenglcomputeshader.h"

namespace vir
{

bool OpenGLComputeShader::firstWaitSyncCall_ = false;
GLsync OpenGLComputeShader::dataSync_;

OpenGLComputeShader::~OpenGLComputeShader()
{
    glDeleteProgram(id_);
}

void OpenGLComputeShader::compile()
{
    // Compile shader
    const char* sourceCstr = source_.c_str();
    GLuint computeShader0 = glCreateShader(GL_COMPUTE_SHADER);
    glShaderSource(computeShader0, 1, &sourceCstr, NULL);
    glCompileShader(computeShader0);
    GLint logLength0 = 0;
    glGetShaderiv(computeShader0, GL_INFO_LOG_LENGTH, &logLength0);
    if (logLength0 > 0)
    {
        std::vector<GLchar> logv(logLength0);
        glGetShaderInfoLog(computeShader0, logLength0, &logLength0, &logv[0]);
        std::string log(logv.begin(), logv.end());
        glDeleteShader(computeShader0);
        std::cout << log << std::endl;
        throw std::runtime_error("[CS0] "+log);
    }
    id_ = glCreateProgram();
    glAttachShader(id_, computeShader0);
    glLinkProgram(id_);
    GLint logLength = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &logLength);
    if (logLength > 0)
    {
        std::vector<GLchar> logv(logLength);
        glGetProgramInfoLog(id_, logLength, &logLength, &logv[0]);
        std::string log(logv.begin(), logv.end());
        glDeleteProgram(id_);
        std::cout << log << std::endl;
        throw std::runtime_error("[CSP] "+log);
    }
    glDeleteShader(computeShader0);
}

GLint OpenGLComputeShader::getUniformLocation
(
    std::string& uniformName
)
{
    GLint location;
    if (uniformLocations_.find(uniformName) != uniformLocations_.end())
        location = (GLint)(uniformLocations_.at(uniformName));
    else
    {
        location = glGetUniformLocation(id_, uniformName.c_str());
        if (location != -1)
            uniformLocations_[uniformName] = location;
    }
    return location;
}

void OpenGLComputeShader::setUniformInt
(
    std::string uniformName, 
    int value,
    bool autoUse
)
{
    if (autoUse)
        use();
    glUniform1i(getUniformLocation(uniformName), value);
}

void OpenGLComputeShader::setUniformFloat
(
    std::string uniformName, 
    float value,
    bool autoUse
)
{
    if (autoUse)
        use();
    glUniform1f(getUniformLocation(uniformName), value);
}

void OpenGLComputeShader::use()
{
    glUseProgram(id_);
}

void OpenGLComputeShader::run
(
    int x, 
    int y, 
    int z, 
    GLbitfield barriers
)
{
    glDispatchCompute(x, y, z);
    glMemoryBarrier(barriers);
}

void OpenGLComputeShader::waitSync()
{
    if (firstWaitSyncCall_)
    {   
        dataSync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
        firstWaitSyncCall_ = false;
    }
    while (dataSync_)
    {
        GLenum wait = glClientWaitSync(dataSync_, 
            GL_SYNC_FLUSH_COMMANDS_BIT, 1);
        if (wait == GL_ALREADY_SIGNALED || wait == GL_CONDITION_SATISFIED)
            break;
    }
}

void OpenGLComputeShader::resetSync()
{
    if (dataSync_)
        glDeleteSync(dataSync_);
    dataSync_ = glFenceSync(GL_SYNC_GPU_COMMANDS_COMPLETE, 0);
}

}
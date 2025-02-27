#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglcomputeshader.h"

namespace vir
{

OpenGLComputeShader::~OpenGLComputeShader()
{
    if (id_ != 0)
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

void OpenGLComputeShader::setUniformInt2
(
    std::string uniformName, 
    glm::ivec2 value,
    bool autoUse
)
{
    if (autoUse)
        use();
    glUniform2iv(getUniformLocation(uniformName), 1, glm::value_ptr(value));
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

void OpenGLComputeShader::setUniformFloat2
(
    std::string uniformName, 
    glm::vec2 value,
    bool autoUse
)
{
    if (autoUse)
        use();
    glUniform2fv(getUniformLocation(uniformName), 1, glm::value_ptr(value));
}

void OpenGLComputeShader::bindShaderStorageBlock
(
    const std::string& blockName,  
    GLuint bindingPoint
)
{
    GLint location = glGetProgramResourceIndex
    (
        id_,
        GL_SHADER_STORAGE_BLOCK,
        blockName.c_str()
    );
    if (location == -1)
        return;
    glShaderStorageBlockBinding(id_, location, bindingPoint);
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

}
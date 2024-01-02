#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglshader.h"

namespace vir
{

// Protected member functions ------------------------------------------------//

void OpenGLShader::checkValidShader
(
    const unsigned int& shader, 
    std::string logPrefix
)
{
    GLint valid;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &valid);
    if (valid == GL_FALSE)
    {
        GLint logLength = 0;
        glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
        std::vector<GLchar> logv(logLength);
        glGetShaderInfoLog(shader, logLength, &logLength, &logv[0]);
        std::string log(logv.begin(), logv.end());
        glDeleteShader(shader);
        throw std::runtime_error(logPrefix+log);
    }
}

unsigned int OpenGLShader::createShaderFromFile
(
    const std::string& filename, 
    GLuint shaderType
)
{
    std::ifstream ifstream(filename);
    std::stringstream sstream;
    sstream << ifstream.rdbuf();
    std::string s = sstream.str();
    return createShaderFromString(s, shaderType);
}

unsigned int OpenGLShader::createShaderFromString
(
    const std::string& sourceString, 
    GLuint shaderType
)
{
    const char* source = sourceString.c_str();
    unsigned int shader = glCreateShader(shaderType); 
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    std::string logPrefix = "";
    if (shaderType == GL_VERTEX_SHADER)
        logPrefix = "[V] ";
    else if (shaderType == GL_FRAGMENT_SHADER)
        logPrefix = "[F] ";
    checkValidShader(shader, logPrefix);
    return shader;
}

// Public member functions ---------------------------------------------------//

OpenGLShader::OpenGLShader
(
    const std::string& vertextShaderSource,
    const std::string& fragmentShaderSource,
    OpenGLShader::ConstructFrom cf
)
{
    unsigned int vertexShader;
    unsigned int fragmentShader;
    switch(cf)
    {
        case ConstructFrom::File :
        {
            vertexShader = createShaderFromFile
            (
                vertextShaderSource, 
                GL_VERTEX_SHADER
            );
            fragmentShader = createShaderFromFile
            (
                fragmentShaderSource, 
                GL_FRAGMENT_SHADER
            );
            break;
        }
        case ConstructFrom::String :
        {
            vertexShader = createShaderFromString
            (
                vertextShaderSource, 
                GL_VERTEX_SHADER
            );
            fragmentShader = createShaderFromString
            (
                fragmentShaderSource, 
                GL_FRAGMENT_SHADER
            );
            break;
        }
    }
    id_ = glCreateProgram();
    glAttachShader(id_, vertexShader);
    glAttachShader(id_, fragmentShader);
    glLinkProgram(id_);
    /*
    GLint valid;
    glGetProgramiv(id_, GL_COMPILE_STATUS, &valid);
    GLint logLength = 0;
    glGetProgramiv(id_, GL_INFO_LOG_LENGTH, &logLength);
    if (valid == GL_FALSE && logLength > 0)
    {
        std::vector<GLchar> logv(logLength);
        glGetProgramInfoLog(id_, logLength, &logLength, &logv[0]);
        std::string log(logv.begin(), logv.end());
        glDeleteProgram(id_);
        throw std::runtime_error("[P] "+log);
    }*/
    glDeleteShader(fragmentShader);
    glDeleteShader(vertexShader);
}

OpenGLShader::~OpenGLShader()
{
    glDeleteProgram(id_);
}

void OpenGLShader::bind() const
{
    glUseProgram(id_);
}

void OpenGLShader::unbind()const
{
    glUseProgram(0);
}

GLint OpenGLShader::getUniformLocation(std::string& name)
{
    if (uniformMap_.find(name) != uniformMap_.end())
        return (GLint)(uniformMap_.at(name));
    GLint location = glGetUniformLocation(id_, name.c_str());
    if (location != -1)
        uniformMap_[name] = location;
    return location;
}

GLint OpenGLShader::getUniformBlockIndex(std::string& name)
{
    if (uniformMap_.find(name) != uniformMap_.end())
        return (GLint)(uniformMap_.at(name));
    GLint location = glGetUniformBlockIndex(id_, name.c_str());
    if (location != -1)
        uniformMap_[name] = location;
    return location;
}

void OpenGLShader::setUniformBool(std::string name, bool value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1i(location, value);
}

void OpenGLShader::setUniformUInt(std::string name, uint32_t value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1ui(location, value);
}

void OpenGLShader::setUniformInt(std::string name, int value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1i(location, value);
}

void OpenGLShader::setUniformInt2(std::string name, glm::ivec2 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform2iv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformInt3(std::string name, glm::ivec3 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform3iv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformInt4(std::string name, glm::ivec4 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform4iv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformFloat(std::string name, float value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1f(location, value);
}

void OpenGLShader::setUniformFloat2(std::string name, glm::vec2 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform2fv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformFloat3(std::string name, glm::vec3 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform3fv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformFloat4(std::string name, glm::vec4 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform4fv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformMat3(std::string name, glm::mat3 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::setUniformMat4(std::string name, glm::mat4 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::bindUniformBlock
(
    std::string name,
    UniformBuffer& ubo,
    uint32_t uboBindingPoint
)
{
    GLint location = getUniformBlockIndex(name);
    if (location == -1)
        return;
    ubo.bind(uboBindingPoint);
    glUniformBlockBinding(id_, ubo.id(), uboBindingPoint);
}

}
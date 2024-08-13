#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglshader.h"

namespace vir
{

// Protected member functions ------------------------------------------------//

void OpenGLShader::checkValidShader
(
    const unsigned int& shader,
    std::string& log
)
{
    GLint valid;
    glGetShaderiv(shader, GL_COMPILE_STATUS, &valid);
    if (valid != GL_FALSE)
    {
        log.clear();
        return;
    }
    GLint logLength = 0;
    glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &logLength);
    std::vector<GLchar> logv(logLength);
    glGetShaderInfoLog(shader, logLength, &logLength, &logv[0]);
    log = std::string(logv.begin(), logv.end());
    glDeleteShader(shader);
}

//----------------------------------------------------------------------------//

unsigned int OpenGLShader::createShaderFromFile
(
    const std::string& filepath, 
    GLuint shaderType,
    std::map<int, std::string>& errors
)
{
    std::ifstream ifstream(filepath);
    std::stringstream sstream;
    sstream << ifstream.rdbuf();
    std::string source = sstream.str();
    return createShaderFromSource(source, shaderType, errors);
}

//----------------------------------------------------------------------------//

unsigned int OpenGLShader::createShaderFromSource
(
    const std::string& sourceString, 
    GLuint shaderType,
    std::map<int, std::string>& errors
)
{
    const char* source = sourceString.c_str();
    unsigned int shader = glCreateShader(shaderType);
    glShaderSource(shader, 1, &source, NULL);
    glCompileShader(shader);
    std::string log;
    checkValidShader(shader, log);
    parseCompilationErrorLog(log, errors);
    return shader;
}

//----------------------------------------------------------------------------//

void OpenGLShader::parseCompilationErrorLog
(
    const std::string& log,
    std::map<int, std::string>& errors
)
{
    if (log.size() == 0)
        return;
    errors.clear();
    bool        readErrorIndex = true;
    int         i              = 0;
    int         i0             = 0;
    int         logSize        = log.size();
    std::string lineNo;
    // The following parsing "should" work on most NVidia and Intel (integrated
    // graphics) OpenGL implementations. If the parsing fails, the whole log
    // is simply added to the error map at index 0
    bool parsingFailed = false;
    try
    {
        while(i < logSize) 
        {
            int j = std::min(i+1, logSize-1);
            if (readErrorIndex && log[i]=='0' && (log[j]=='(' || log[j]==':'))
            {
                ++i;
                while (log[++i] != ')' && log[i] != ':')
                    lineNo += log[i];
                while (log[++i] == ' ' || log[i] == ':'){}
                i0 = i;
                readErrorIndex = false;
            }
            else if (lineNo.size() > 0 && log[i] == '\n')
            {
                errors.insert({std::stoi(lineNo), log.substr(i0, i-i0)});
                lineNo.clear();
                readErrorIndex = true;
            }
            ++i;
        }
    }
    catch(...)
    {
        parsingFailed = true;
    }
    if (errors.size() == 0 || parsingFailed)
        errors.insert({0, log});
}

// Public member functions ---------------------------------------------------//

OpenGLShader::OpenGLShader
(
    const std::string& vertextShaderSource,
    const std::string& fragmentShaderSource,
    OpenGLShader::ConstructFrom cf
)
{
    static bool currentContextExtensionStatusMapInitialized = false;
    if (!currentContextExtensionStatusMapInitialized)
    {
        static auto* context = vir::GlobalPtr<vir::Window>::instance()->context();
        for (const auto& extension : context->supportedExtensions())
            currentContextExtensionsStatusMap_.insert({extension, false});
        currentContextExtensionStatusMapInitialized = true;
    }

    unsigned int vertexShader;
    unsigned int fragmentShader;
    switch(cf)
    {
        case ConstructFrom::SourceFile :
        {
            vertexShader = createShaderFromFile
            (
                vertextShaderSource, 
                GL_VERTEX_SHADER,
                compilationErrors_.vertexErrors
            );
            fragmentShader = createShaderFromFile
            (
                fragmentShaderSource, 
                GL_FRAGMENT_SHADER,
                compilationErrors_.fragmentErrors
            );
            break;
        }
        case ConstructFrom::SourceCode :
        {
            vertexShader = createShaderFromSource
            (
                vertextShaderSource, 
                GL_VERTEX_SHADER,
                compilationErrors_.vertexErrors
            );
            fragmentShader = createShaderFromSource
            (
                fragmentShaderSource, 
                GL_FRAGMENT_SHADER,
                compilationErrors_.fragmentErrors
            );
            break;
        }
    }
    if (valid())
    {
        id_ = glCreateProgram();
        glAttachShader(id_, vertexShader);
        glAttachShader(id_, fragmentShader);
        glLinkProgram(id_);
    }
    /*GLint valid;
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

GLint OpenGLShader::getUniformLocation(const std::string& name)
{
    if (uniformMap_.find(name) != uniformMap_.end())
        return (GLint)(uniformMap_.at(name));
    GLint location = glGetUniformLocation(id_, name.c_str());
    if (location != -1)
        uniformMap_[name] = location;
    return location;
}

GLint OpenGLShader::getUniformBlockIndex(const std::string& name)
{
    if (uniformMap_.find(name) != uniformMap_.end())
        return (GLint)(uniformMap_.at(name));
    GLint location = glGetUniformBlockIndex(id_, name.c_str());
    if (location != -1)
        uniformMap_[name] = location;
    return location;
}

void OpenGLShader::setUniformBool(const std::string& name, bool value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1i(location, value);
}

void OpenGLShader::setUniformUInt(const std::string& name, uint32_t value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1ui(location, value);
}

void OpenGLShader::setUniformInt(const std::string& name, int value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1i(location, value);
}

void OpenGLShader::setUniformInt2(const std::string& name, glm::ivec2 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform2iv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformInt3(const std::string& name, glm::ivec3 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform3iv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformInt4(const std::string& name, glm::ivec4 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform4iv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformFloat(const std::string& name, float value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform1f(location, value);
}

void OpenGLShader::setUniformFloat2(const std::string& name, glm::vec2 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform2fv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformFloat3(const std::string& name, glm::vec3 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform3fv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformFloat4(const std::string& name, glm::vec4 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniform4fv(location, 1, glm::value_ptr(value));
}

void OpenGLShader::setUniformMat3(const std::string& name, glm::mat3 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniformMatrix3fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::setUniformMat4(const std::string& name, glm::mat4 value)
{
    GLint location = getUniformLocation(name);
    if (location != -1)
        glUniformMatrix4fv(location, 1, GL_FALSE, glm::value_ptr(value));
}

void OpenGLShader::bindUniformBlock
(
    const std::string& blockName,
    uint32_t bindingPoint
)
{
    GLint location = getUniformBlockIndex(blockName);
    if (location == -1)
        return;
    glUniformBlockBinding(id_, location, bindingPoint);
}

void OpenGLShader::bindShaderStorageBlock
(
    const std::string& blockName,  
    uint32_t bindingPoint
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

//----------------------------------------------------------------------------//

std::string OpenGLShader::currentContextShadingLanguageDirectives()
{
    static auto* context = vir::GlobalPtr<vir::Window>::instance()->context();
    std::string version = 
        std::to_string(context->versionMajor()) +
        std::to_string(context->versionMinor()) +
        "0";
    std::string directives =
        "#version "+version+" core\n";
    for (auto item : currentContextExtensionsStatusMap_)
    {
        if (!item.second)
            continue;
        directives += 
            "#ifdef "+item.first+"\n#extension "+item.first+
            " : enable\n#endif\n";
    }
    return directives;
}

bool OpenGLShader::setExtensionStatusInCurrentContextShadingLanguageDirectives
(
    const std::string& extensionName,
    bool status
)
{
    static auto* context = vir::GlobalPtr<vir::Window>::instance()->context();
    if (!context->isExtensionSupported(extensionName))
        return false;
    currentContextExtensionsStatusMap_[extensionName] = status;
    return true;
}

}
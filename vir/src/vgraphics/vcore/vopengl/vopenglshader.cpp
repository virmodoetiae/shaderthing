#include "vpch.h"
#include "vgraphics/vcore/vopengl/vopenglshader.h"

#include <thirdparty/glslang/glslang/Public/ShaderLang.h>
#include <thirdparty/glslang/SPIRV/GlslangToSpv.h>
#include <thirdparty/glslang/glslang/Public/ResourceLimits.h>

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
    const TBuiltInResource DefaultTBuiltInResource = {
    /* .MaxLights = */ 32,
    /* .MaxClipPlanes = */ 6,
    /* .MaxTextureUnits = */ 32,
    /* .MaxTextureCoords = */ 32,
    /* .MaxVertexAttribs = */ 64,
    /* .MaxVertexUniformComponents = */ 4096,
    /* .MaxVaryingFloats = */ 64,
    /* .MaxVertexTextureImageUnits = */ 32,
    /* .MaxCombinedTextureImageUnits = */ 80,
    /* .MaxTextureImageUnits = */ 32,
    /* .MaxFragmentUniformComponents = */ 4096,
    /* .MaxDrawBuffers = */ 32,
    /* .MaxVertexUniformVectors = */ 128,
    /* .MaxVaryingVectors = */ 8,
    /* .MaxFragmentUniformVectors = */ 16,
    /* .MaxVertexOutputVectors = */ 16,
    /* .MaxFragmentInputVectors = */ 15,
    /* .MinProgramTexelOffset = */ -8,
    /* .MaxProgramTexelOffset = */ 7,
    /* .MaxClipDistances = */ 8,
    /* .MaxComputeWorkGroupCountX = */ 65535,
    /* .MaxComputeWorkGroupCountY = */ 65535,
    /* .MaxComputeWorkGroupCountZ = */ 65535,
    /* .MaxComputeWorkGroupSizeX = */ 1024,
    /* .MaxComputeWorkGroupSizeY = */ 1024,
    /* .MaxComputeWorkGroupSizeZ = */ 64,
    /* .MaxComputeUniformComponents = */ 1024,
    /* .MaxComputeTextureImageUnits = */ 16,
    /* .MaxComputeImageUniforms = */ 8,
    /* .MaxComputeAtomicCounters = */ 8,
    /* .MaxComputeAtomicCounterBuffers = */ 1,
    /* .MaxVaryingComponents = */ 60,
    /* .MaxVertexOutputComponents = */ 64,
    /* .MaxGeometryInputComponents = */ 64,
    /* .MaxGeometryOutputComponents = */ 128,
    /* .MaxFragmentInputComponents = */ 128,
    /* .MaxImageUnits = */ 8,
    /* .MaxCombinedImageUnitsAndFragmentOutputs = */ 8,
    /* .MaxCombinedShaderOutputResources = */ 8,
    /* .MaxImageSamples = */ 0,
    /* .MaxVertexImageUniforms = */ 0,
    /* .MaxTessControlImageUniforms = */ 0,
    /* .MaxTessEvaluationImageUniforms = */ 0,
    /* .MaxGeometryImageUniforms = */ 0,
    /* .MaxFragmentImageUniforms = */ 8,
    /* .MaxCombinedImageUniforms = */ 8,
    /* .MaxGeometryTextureImageUnits = */ 16,
    /* .MaxGeometryOutputVertices = */ 256,
    /* .MaxGeometryTotalOutputComponents = */ 1024,
    /* .MaxGeometryUniformComponents = */ 1024,
    /* .MaxGeometryVaryingComponents = */ 64,
    /* .MaxTessControlInputComponents = */ 128,
    /* .MaxTessControlOutputComponents = */ 128,
    /* .MaxTessControlTextureImageUnits = */ 16,
    /* .MaxTessControlUniformComponents = */ 1024,
    /* .MaxTessControlTotalOutputComponents = */ 4096,
    /* .MaxTessEvaluationInputComponents = */ 128,
    /* .MaxTessEvaluationOutputComponents = */ 128,
    /* .MaxTessEvaluationTextureImageUnits = */ 16,
    /* .MaxTessEvaluationUniformComponents = */ 1024,
    /* .MaxTessPatchComponents = */ 120,
    /* .MaxPatchVertices = */ 32,
    /* .MaxTessGenLevel = */ 64,
    /* .MaxViewports = */ 16,
    /* .MaxVertexAtomicCounters = */ 0,
    /* .MaxTessControlAtomicCounters = */ 0,
    /* .MaxTessEvaluationAtomicCounters = */ 0,
    /* .MaxGeometryAtomicCounters = */ 0,
    /* .MaxFragmentAtomicCounters = */ 8,
    /* .MaxCombinedAtomicCounters = */ 8,
    /* .MaxAtomicCounterBindings = */ 1,
    /* .MaxVertexAtomicCounterBuffers = */ 0,
    /* .MaxTessControlAtomicCounterBuffers = */ 0,
    /* .MaxTessEvaluationAtomicCounterBuffers = */ 0,
    /* .MaxGeometryAtomicCounterBuffers = */ 0,
    /* .MaxFragmentAtomicCounterBuffers = */ 1,
    /* .MaxCombinedAtomicCounterBuffers = */ 1,
    /* .MaxAtomicCounterBufferSize = */ 16384,
    /* .MaxTransformFeedbackBuffers = */ 4,
    /* .MaxTransformFeedbackInterleavedComponents = */ 64,
    /* .MaxCullDistances = */ 8,
    /* .MaxCombinedClipAndCullDistances = */ 8,
    /* .MaxSamples = */ 4,
    /* .maxMeshOutputVerticesNV = */ 256,
    /* .maxMeshOutputPrimitivesNV = */ 512,
    /* .maxMeshWorkGroupSizeX_NV = */ 32,
    /* .maxMeshWorkGroupSizeY_NV = */ 1,
    /* .maxMeshWorkGroupSizeZ_NV = */ 1,
    /* .maxTaskWorkGroupSizeX_NV = */ 32,
    /* .maxTaskWorkGroupSizeY_NV = */ 1,
    /* .maxTaskWorkGroupSizeZ_NV = */ 1,
    /* .maxMeshViewCountNV = */ 4,
    /* .maxMeshOutputVerticesEXT = */ 256,
    /* .maxMeshOutputPrimitivesEXT = */ 256,
    /* .maxMeshWorkGroupSizeX_EXT = */ 128,
    /* .maxMeshWorkGroupSizeY_EXT = */ 128,
    /* .maxMeshWorkGroupSizeZ_EXT = */ 128,
    /* .maxTaskWorkGroupSizeX_EXT = */ 128,
    /* .maxTaskWorkGroupSizeY_EXT = */ 128,
    /* .maxTaskWorkGroupSizeZ_EXT = */ 128,
    /* .maxMeshViewCountEXT = */ 4,
    /* .maxDualSourceDrawBuffersEXT = */ 1,
    
    /* .limits = */ {
        /* .nonInductiveForLoops = */ 1,
        /* .whileLoops = */ 1,
        /* .doWhileLoops = */ 1,
        /* .generalUniformIndexing = */ 1,
        /* .generalAttributeMatrixVectorIndexing = */ 1,
        /* .generalVaryingIndexing = */ 1,
        /* .generalSamplerIndexing = */ 1,
        /* .generalVariableIndexing = */ 1,
        /* .generalConstantMatrixVectorIndexing = */ 1,
    }};

    
    EShLanguage stage = shaderType == GL_VERTEX_SHADER ? 
        EShLangVertex : EShLangFragment;
    glslang::TShader glslangShader(stage);
    const char* shaderStrings[] = { sourceString.c_str() };
    glslangShader.setStrings(shaderStrings, 1);
    glslangShader.setEnvInput(glslang::EShSource::EShSourceGlsl, stage, glslang::EShClient::EShClientOpenGL, 460);
    glslangShader.setEnvClient(glslang::EShClient::EShClientOpenGL, glslang::EShTargetClientVersion::EShTargetOpenGL_450);
    glslangShader.setEnvTarget(glslang::EShTargetLanguage::EShTargetSpv, glslang::EShTargetLanguageVersion::EShTargetSpv_1_6);

    const TBuiltInResource resources = DefaultTBuiltInResource;
    // Preprocess and parse shader
    EShMessages messages = (EShMessages)(EShMsgSpvRules | EShMsgVulkanRules);
    std::string preprocessedGLSL;
    
    // Parse shader
    if (!glslangShader.parse(&resources, 100, false, messages)) {
        // Handle compilation error
        std::string infoLog = glslangShader.getInfoLog();
        // parseCompilationErrorLog(infoLog, errors);
        int breakpoint = 0;
    }

    std::vector<unsigned int> spirv;
    glslang::GlslangToSpv(*glslangShader.getIntermediate(), spirv);

    //const char* source = sourceString.c_str();
    unsigned int shader = glCreateShader(shaderType);

    glShaderBinary(1, &shader, GL_SHADER_BINARY_FORMAT_SPIR_V_ARB, spirv.data(), spirv.size() * sizeof(unsigned int));
    glSpecializeShaderARB(shader, "main", 0, nullptr, nullptr);
    //glShaderSource(shader, 1, &source, NULL);
    //glCompileShader(shader);
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
                int iLine = std::stoi(lineNo);
                if (errors.find(iLine) == errors.end())
                    errors.insert({iLine, log.substr(i0, i-i0)});
                else
                    errors[iLine] += "\n"+log.substr(i0, i-i0);
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
    {
        errors.clear();
        errors.insert({0, log});
    }
}

// Public member functions ---------------------------------------------------//

OpenGLShader::OpenGLShader
(
    const std::string& vertextShaderSource,
    const std::string& fragmentShaderSource,
    OpenGLShader::ConstructFrom cf
)
{
    glslang::InitializeProcess();
    static bool currentContextExtensionStatusMapInitialized = false;
    if (!currentContextExtensionStatusMapInitialized)
    {
        static auto* context = vir::GlobalPtr<vir::Window>::instance()->context();
        for (const auto& extension : context->supportedExtensions())
            currentContextExtensionsStatusMap_.insert({extension, false});
        currentContextExtensionStatusMapInitialized = true;
    }

    unsigned int vertexShader = 0;
    unsigned int fragmentShader = 0;
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
    glslang::FinalizeProcess();
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
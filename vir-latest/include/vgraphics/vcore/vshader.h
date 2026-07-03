#ifndef VSHADER_H
#define VSHADER_H

#include <string>
#include <unordered_map>
#include <variant>
#include <vector>

#include "vir/include/vmacros.h"
#include "vir/include/vpointers.h"
#include "vir/include/vgraphics/vcore/vuniform.h"

namespace vir
{

class Shader
{
public:

    enum class ConstructFrom
    {
        // The passed string is the source code itself
        SourceCode,
        // The passed string is a filepath to the source code
        SourceFile 
    };

    struct CompilationErrors
    {
        std::map<int, std::string> vertexErrors   = {};
        std::map<int, std::string> fragmentErrors = {};
        unsigned int size() const 
        {
            return vertexErrors.size() + fragmentErrors.size();
        }
    };

    static std::unordered_map<std::string, Uniform::Type> 
        valueTypeToUniformTypeMap;
    static std::unordered_map<Uniform::Type, std::string>
        uniformTypeToName;
    static std::unordered_map<std::string, Uniform::Type>
        uniformNameToType;
    static std::vector<std::string> uniformNames;
    static std::vector<Uniform::Type> uniformTypes;

protected:
    uint32_t id_;
    std::unordered_map<std::string, uint32_t> uniformMap_;
    CompilationErrors compilationErrors_;
    // Map of all extensions supported by the graphics context to their 
    // respective status in the shading language, i.e., is said extension
    // included in the shading language directives returned by 
    // shadingLanguageDirectives()? Only useful for OpenGL (as far as I know)
    static std::unordered_map<std::string, bool> 
        currentContextExtensionsStatusMap_;
    Shader() = default;
    DELETE_COPY_MOVE(Shader)
public:
    static UniquePtr<Shader> create
    (
        const std::string& vertexSource, 
        const std::string& fragmentSource, 
        ConstructFrom constructFrom
    ); 
    virtual ~Shader() = default;
    virtual void bind() const = 0;
    virtual void unbind() const = 0;

    virtual void setUniformBool  (const std::string&, bool) = 0;
    virtual void setUniformUInt  (const std::string&, uint32_t) = 0;
    virtual void setUniformInt   (const std::string&, int) = 0;
    virtual void setUniformInt2  (const std::string&, glm::ivec2) = 0;
    virtual void setUniformInt3  (const std::string&, glm::ivec3) = 0;
    virtual void setUniformInt4  (const std::string&, glm::ivec4) = 0;
    virtual void setUniformFloat (const std::string&, float) = 0;
    virtual void setUniformFloat2(const std::string&, glm::vec2) = 0;
    virtual void setUniformFloat3(const std::string&, glm::vec3) = 0;
    virtual void setUniformFloat4(const std::string&, glm::vec4) = 0;
    virtual void setUniformMat3  (const std::string&, glm::mat3) = 0;
    virtual void setUniformMat4  (const std::string&, glm::mat4) = 0;

    // Locates and binds a named uniform block in this shader to the provided
    // bindingPoint
    virtual void bindUniformBlock
    (
        const std::string& blockName,  
        uint32_t bindingPoint
    ) = 0;

    // Locates and binds a named shader storage block in this shader to the
    // provided bindingPoint
    virtual void bindShaderStorageBlock
    (
        const std::string& blockName,  
        uint32_t bindingPoint
    ) = 0;

    uint32_t id() const {return id_;}
    const CompilationErrors& compilationErrors() const 
    {
        return compilationErrors_;
    }
    bool valid() const {return compilationErrors_.size() == 0;}

    static std::string currentContextShadingLanguageDirectives();
    static bool setExtensionStatusInCurrentContextShadingLanguageDirectives
    (
        const std::string& extensionName,
        bool status
    );
    static bool isExtensionInCurrentContextShadingLanguageDirectives
    (
        const std::string& extensionName
    );
    static std::vector<std::string> 
        extensionsInCurrentContextShadingLanguageDirectives();
};

}

#endif
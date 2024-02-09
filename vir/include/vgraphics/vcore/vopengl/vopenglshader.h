#ifndef V_OPENGL_SHADER_H
#define V_OPENGL_SHADER_H

#include <string>
#include "thirdparty/glm/glm.hpp"
#include "thirdparty/glad/include/glad/glad.h"
#include "vgraphics/vcore/vshader.h"

namespace vir
{

class OpenGLShader : public Shader
{
protected:
    static void checkValidShader(const unsigned int& id, std::string& log);
    static unsigned int createShaderFromSource(const std::string& source, GLuint type, std::map<int, std::string>& errors);
    static unsigned int createShaderFromFile(const std::string& filepath, GLuint type, std::map<int, std::string>& errors);
    static void parseCompilationErrorLog(const std::string& log, std::map<int, std::string>& errors);
public:
    // Construct of from vertex and fragment shader (either source code or file
    // path)
    OpenGLShader(const std::string& v, const std::string& f, ConstructFrom c);
    ~OpenGLShader() override;
    void bind() const override;
    void unbind() const override;

    void setUniformBool(std::string, bool) override;
    void setUniformUInt(std::string, uint32_t) override;
    void setUniformInt(std::string, int) override;
    void setUniformInt2(std::string, glm::ivec2) override;
    void setUniformInt3(std::string, glm::ivec3) override;
    void setUniformInt4(std::string, glm::ivec4) override;
    void setUniformFloat(std::string, float) override;
    void setUniformFloat2(std::string, glm::vec2) override;
    void setUniformFloat3(std::string, glm::vec3) override;
    void setUniformFloat4(std::string, glm::vec4) override;
    void setUniformMat3(std::string, glm::mat3) override;
    void setUniformMat4(std::string, glm::mat4) override;

    void bindUniformBlock
    (
        std::string name, 
        UniformBuffer& ubo, 
        uint32_t uboBindingPoint=0
    ) override;

    GLint getUniformLocation(std::string&);
    GLint getUniformBlockIndex(std::string&);
};

}

#endif
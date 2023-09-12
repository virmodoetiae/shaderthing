#ifndef V_OPENGL_SHADER_H
#define V_OPENGL_SHADER_H

#include <string>
#include "thirdparty/glm/glm.hpp"
#include "thirdparty/glad/include/glad/glad.h"
#include "vgraphics/vshader.h"

namespace vir
{

class OpenGLShader : public Shader
{
protected:
    void checkValidShader(const unsigned int&, std::string logPrefix="");
    unsigned int createShaderFromString(const std::string&, GLuint);
    unsigned int createShaderFromFile(const std::string&, GLuint);
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

    GLint getUniformLocation(std::string&);
};

}

#endif
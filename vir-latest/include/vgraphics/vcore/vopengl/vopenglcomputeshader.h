#ifndef V_OPENGL_COMPUTE_SHADER_H
#define V_OPENGL_COMPUTE_SHADER_H

#include <string>
#include <unordered_map>

#include "thirdparty/glad/include/glad/glad.h"

namespace vir
{

class OpenGLComputeShader
{
protected:
    GLuint id_;
    std::string source_;
    std::unordered_map<std::string, GLint> uniformLocations_;
public:
    OpenGLComputeShader(std::string source):id_(0),source_(source){}
    ~OpenGLComputeShader();
    void compile();
    GLint getUniformLocation(std::string& uniformName);
    void setUniformInt(std::string uniformName,int value,bool autoUse=true);
    void setUniformInt2
    (
        std::string uniformName,
        glm::ivec2 value,
        bool autoUse=true
    );
    void setUniformFloat
    (
        std::string uniformName,
        float value,
        bool autoUse=true
    );
    void setUniformFloat2
    (
        std::string uniformName,
        glm::vec2 value,
        bool autoUse=true
    );
    void bindShaderStorageBlock
    (
        const std::string& blockName,  
        GLuint bindingPoint
    );
    void use();
    void run(int x, int y, int z, GLbitfield barriers=GL_ALL_BARRIER_BITS);
};

}

#endif
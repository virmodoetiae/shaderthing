#ifndef V_GRAPHICS_CONTEXT
#define V_GRAPHICS_CONTEXT

#include <iostream>
#include <string>

namespace vir
{

class GraphicsContext
{
public:
    enum class Type
    {
        OpenGL
    };
protected:
    std::string name_;
    int versionMajor_;
    int versionMinor_;
    std::vector<std::string> supportedExtensions_;
public:
    GraphicsContext() = default;
    virtual ~GraphicsContext(){}

    const std::string& name() const {return name_;}
    int versionMajor() const {return versionMajor_;}
    int versionMinor() const {return versionMinor_;}
    const std::vector<std::string>& supportedExtensions() const
    {
        return supportedExtensions_;
    }
    bool isExtensionSupported(const std::string& extensionName) const
    {
        return std::find
        (
            supportedExtensions_.begin(),
            supportedExtensions_.end(),
            extensionName
        ) != supportedExtensions_.end();
    }
    virtual Type type() const = 0;
    virtual void initialize(void* nativeWindow) = 0;
    virtual void printErrors() const = 0;
};

}

#endif
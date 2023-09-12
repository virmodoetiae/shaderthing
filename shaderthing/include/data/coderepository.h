#ifndef ST_CODE_REPOSITORY_H
#define ST_CODE_REPOSITORY_H

#include <string>

namespace ShaderThing
{

class CodeRepository
{
private:

    bool isGuiOpen_;
    bool isGuiInMenu_;

public:

    CodeRepository();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    // Accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
};

}

#endif
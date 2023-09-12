#ifndef ST_ABOUT_H
#define ST_ABOUT_H

#include <string>

namespace ShaderThing
{

class Resource;

class About
{
private:

    bool isGuiOpen_;
    bool isGuiInMenu_;
    Resource* virmodoetiaeImage_;

public:

    About();
    ~About();

    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();

    // Accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
};

}

#endif
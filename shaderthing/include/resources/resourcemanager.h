/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a, virmodoetiae).
|  |\  \|\__    __\   |  For more information, visit:
|  \ \  \|__|\  \_|   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \|__|\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2023 Stefan Radman
|  Ↄ|C    \|__|\|__|  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#ifndef ST_RESOURCE_MANAGER_H
#define ST_RESOURCE_MANAGER_H

#include <vector>
#include <string>

#include "vir/include/vir.h"

namespace ShaderThing
{

class ShaderThingApp;
class Layer;
class Resource;
class ObjectIO;

class ResourceManager
{
private:

    // Ref to top-level app
    ShaderThingApp& app_;

    // List of managed resources
    std::vector<Resource*> resources_;

    // Is this manager's GUI open?
    bool isGuiOpen_;
    bool isGuiInMenu_;
    
    bool loadOrReplaceTextureOrAnimationGuiButton
    (
        int rowIndex=-1,
        ImVec2 size=ImVec2(0,0),
        bool animation=false,
        bool disabled=false
    );
    bool createOrEditAnimationGuiButton
    (
        int rowIndex=-1,
        ImVec2 size=ImVec2(0,0)
    );
    bool createOrEditCubemapGuiButton
    (
        int rowIndex=-1,
        ImVec2 size=ImVec2(0,0)
    );
    bool exportTextureOrAnimationGuiButton
    (
        Resource* resource,
        ImVec2 size=ImVec2(0,0)
    );

public:

    ResourceManager(ShaderThingApp& app);
    virtual ~ResourceManager();

    void reset();
    void update();

    void addLayerAsResource(Layer* layer);
    void removeLayerAsResource(Layer* layer);
    
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();
    void loadState(ObjectIO& reader);
    void saveState(ObjectIO& writer);

    // Accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
    std::vector<Resource*>& resourcesRef(){return resources_;}
};

}

#endif
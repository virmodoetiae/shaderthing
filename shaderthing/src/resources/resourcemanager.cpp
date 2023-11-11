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

#include "shaderthingapp.h"
#include "layers/layer.h"
#include "layers/layermanager.h"
#include "resources/resource.h"
#include "resources/resourcemanager.h"
#include "data/data.h"
#include "objectio/objectio.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/stb/stb_image.h"
#include "thirdparty/rapidjson/include/rapidjson/document.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

// Constructor/destructor ----------------------------------------------------//

ResourceManager::ResourceManager(ShaderThingApp& app) : 
app_(app),
resources_(0),
isGuiOpen_(false),
isGuiInMenu_(true)
{}

ResourceManager::~ResourceManager()
{
    for (auto r : resources_)
        delete r;
    resources_.resize(0);
    isGuiOpen_ = false;
}

//----------------------------------------------------------------------------//

void ResourceManager::reset()
{
    for (auto r : resources_)
        delete r;
    resources_.resize(0);
}                                                       

//----------------------------------------------------------------------------//

void ResourceManager::loadState(ObjectIO& reader)
{
    // Remove currently managed resources
    reset();
    auto resources= reader.readObject("resources");
    for (auto resourceName : resources.members())
    {   
        auto resource = resources.readObject(resourceName);
        resources_.emplace_back(new Resource(resource, resources_));
    };
}

//----------------------------------------------------------------------------//

void ResourceManager::saveState(ObjectIO& writer)
{
    writer.writeObjectStart("resources");
    for (auto r : resources_)
        r->saveState(writer);
    writer.writeObjectEnd();
}

//----------------------------------------------------------------------------//

void ResourceManager::addLayerAsResource(Layer* layer)
{
    for (auto r : resources_)
    {
        if (r->type() != Resource::Type::FramebufferColorAttachment)
            continue;
        if (r->id() == layer->readOnlyFramebuffer()->colorBufferId())
            return;
    }
    auto resource = new Resource();
    resource->set(&layer->readOnlyFramebuffer());
    resource->setNamePtr(&layer->nameRef());
    resources_.emplace_back(resource);
}

//----------------------------------------------------------------------------//

void ResourceManager::removeLayerAsResource(Layer* layer)
{
    int i = -1;
    bool found = false;
    for (auto r : resources_)
    {
        i++;
        if (r->type() != Resource::Type::FramebufferColorAttachment)
            continue;
        found = (r->id() == layer->readOnlyFramebuffer()->colorBufferId());
        if (found)
        {
            app_.layerManagerRef().removeResourceFromUniforms(r);
            resources_.erase(resources_.begin()+i);
            return;
        }
    }
}

}
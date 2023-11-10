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

#define READ_SOURCE_UP_TO_NEW_LINE(storage)                                 \
    while(true)                                                             \
    {                                                                       \
        char& c(source[index]);                                             \
        if (c == '\n')                                                      \
            break;                                                          \
        storage += source[index];                                           \
        ++index;                                                            \
    }                                                                       \
    ++index;                                                                

void ResourceManager::loadState(std::string& source, uint32_t& index)
{
    // Remove currently managed resources
    reset();

    // Load resources from file being loaded
    int nResources;
    std::string tmp;
    READ_SOURCE_UP_TO_NEW_LINE(tmp)
    sscanf(tmp.c_str(), "%d", &nResources);
    tmp.clear();
    for (int i=0; i < nResources; i++)
    {
        READ_SOURCE_UP_TO_NEW_LINE(tmp)
        Resource::Type type = Resource::nameToType[tmp];
        tmp.clear();
        switch(type)
        {
            case Resource::Type::Texture2D :
            {
                loadTexture2DResource(source, index);
                break;
            }
            case Resource::Type::Cubemap :
            {
                loadCubemapResource(source, index);
                break;
            }
        }
    }
}

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

void ResourceManager::saveState(std::ofstream& file)
{
    int nSavableResources(0);
    for (auto r : resources_)
        if 
        (
            r->type() != Resource::Type::Uninitialized &&
            r->type() != Resource::Type::FramebufferColorAttachment
        )
            nSavableResources+=1;
    file << nSavableResources << std::endl;
    for (auto r : resources_)
    {
        switch (r->type())
        {
            case Resource::Type::Uninitialized :
            case Resource::Type::FramebufferColorAttachment :
                continue;
            case Resource::Type::Texture2D :
            {
                file<< Resource::typeToName[r->type()] << std::endl;
                file<< r->name() 
                    << " " << r->originalFileExtension() << " " 
                    << (int)r->wrapMode(0) << " " << (int)r->wrapMode(1) << " "
                    << (int)r->magFilterMode() << " " << (int)r->minFilterMode()
                    << " " << r->rawDataSize() << std::endl;
                file.write((const char*)r->rawData(), r->rawDataSize());
                break;
            }
            case Resource::Type::Cubemap :
            {
 
                file<< Resource::typeToName[r->type()] << std::endl;
                file<< r->name()<<" ";
                for (int i=0; i<6; i++)
                    file << r->referencedResourcesCRef()[i]->name() << " ";
                file<< (int)r->magFilterMode() << " " <<(int)r->minFilterMode();
                break;
            }
        }
        file << std::endl;
    }
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

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

void ResourceManager::loadTexture2DResource
(
    std::string& source, 
    uint32_t& index
)
{
    std::string tmp;
    READ_SOURCE_UP_TO_NEW_LINE(tmp)
    auto name = new char[tmp.size()];
    auto extension = new char[tmp.size()];
    uint32_t xWrap, yWrap, magFilter, minFilter, rawDataSize;
    sscanf
    (
        tmp.c_str(), 
        "%s %s %d %d %d %d %d", 
        &(name[0]), &(extension[0]), 
        &xWrap, &yWrap, &magFilter, &minFilter,
        &rawDataSize
    );
    tmp.clear();
    uint32_t index0(index);
    unsigned char* rawData = new unsigned char[rawDataSize];
    while(index-index0 < rawDataSize)
    {
        char& c(source[index]);
        rawData[index-index0] = (unsigned char)c;
        ++index;
    }
    ++index;
    auto r = new Resource();
    if (!r->set(rawData, rawDataSize))
    {
        delete r;
        return;
    }
    resources_.push_back(r);
    r->setRawData(rawData, rawDataSize);
    r->setNamePtr(new std::string(name));
    delete[] name;
    r->setOriginalFileExtension(std::string(extension));
    delete[] extension;
    r->setWrapMode(0, (vir::TextureBuffer::WrapMode)xWrap);
    r->setWrapMode(1, (vir::TextureBuffer::WrapMode)yWrap);
    r->setMagFilterMode((vir::TextureBuffer::FilterMode)magFilter);
    r->setMinFilterMode((vir::TextureBuffer::FilterMode)minFilter);
}

//----------------------------------------------------------------------------//

void ResourceManager::loadCubemapResource(std::string& source, uint32_t& index)
{
    std::string tmp;
    READ_SOURCE_UP_TO_NEW_LINE(tmp)
    auto name = new char[tmp.size()];
    auto faceName0 = new char[tmp.size()];
    auto faceName1 = new char[tmp.size()];
    auto faceName2 = new char[tmp.size()];
    auto faceName3 = new char[tmp.size()];
    auto faceName4 = new char[tmp.size()];
    auto faceName5 = new char[tmp.size()];
    uint32_t magFilter, minFilter, rawDataSize;
    sscanf
    (
        tmp.c_str(), 
        "%s %s %s %s %s %s %s %d %d", 
        &(name[0]), 
        &(faceName0[0]),
        &(faceName1[0]),
        &(faceName2[0]),
        &(faceName3[0]),
        &(faceName4[0]),
        &(faceName5[0]),
        &magFilter,
        &minFilter
    );
    tmp.clear();
    char* faceNames[6] = 
    {
        faceName0, 
        faceName1, 
        faceName2, 
        faceName3, 
        faceName4, 
        faceName5
    };
    Resource* textureResources[6];
    for (int i=0;i<6;i++)
    {
        std::string faceName(faceNames[i]);
        for (auto r : resources_)
            if 
            (
                r->name() == faceName && 
                r->type() == Resource::Type::Texture2D
            )
                textureResources[i] = r;
    }
    auto resource = new Resource();
    auto resourceName = new std::string(name);
    resource->set(textureResources);
    resource->setNamePtr(resourceName);
    resource->setMagFilterMode((vir::TextureBuffer::FilterMode)magFilter);
    resource->setMinFilterMode((vir::TextureBuffer::FilterMode)minFilter);
    resources_.emplace_back(resource);
    delete name;
    delete faceName0;
    delete faceName1;
    delete faceName2;
    delete faceName3;
    delete faceName4;
    delete faceName5;
}

}
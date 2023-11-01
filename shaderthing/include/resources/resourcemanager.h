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
    
    void loadTexture2DResource(std::string& source, uint32_t& index);
    void loadCubemapResource(std::string& source, uint32_t& index);
    bool addOrReplaceTextureGuiButton
    (
        int rowIndex=-1,
        ImVec2 size=ImVec2(0,0),
        bool disabled=false
    );
    bool addOrReplaceCubemapGuiButton
    (
        int rowIndex=-1,
        ImVec2 size=ImVec2(0,0)
    );
    bool reExportTextureGuiButton
    (
        Resource* resource,
        ImVec2 size=ImVec2(0,0)
    );

public:

    ResourceManager(ShaderThingApp& app);
    virtual ~ResourceManager();

    void reset();

    void addLayerAsResource(Layer* layer);
    void removeLayerAsResource(Layer* layer);
    
    void toggleIsGuiInMenu(){isGuiInMenu_ = !isGuiInMenu_;}
    void renderGui();
    void loadState(std::string& source, uint32_t& index);
    void saveState(std::ofstream& file);

    // Accessors
    bool* isGuiOpenPtr(){return &isGuiOpen_;}
    bool isGuiInMenu(){return isGuiInMenu_;}
    std::vector<Resource*>& resourcesRef(){return resources_;}

    // Serialization
    template<typename RapidJSONWriterType>
    void saveState(RapidJSONWriterType& writer)
    {
        writer.String("resources");
        writer.StartObject();

        for (auto r : resources_)
        {
            writer.String(r->name().c_str());
            writer.StartObject();
            switch (r->type())
            {
                case Resource::Type::Uninitialized :
                case Resource::Type::FramebufferColorAttachment :
                    continue;
                case Resource::Type::Texture2D :
                {
                    writer.String("type");
                    writer.String(Resource::typeToName[r->type()].c_str());

                    writer.String("originalFileExtension");
                    writer.String(r->originalFileExtension().c_str());

                    writer.String("wrapModes");
                    writer.StartArray();
                    for (int i=0; i<2; i++)
                        writer.Int((int)r->wrapMode(i));
                    writer.EndArray();

                    writer.String("maginificationFilterMode");
                    writer.Int((int)r->magFilterMode());

                    writer.String("minimizationFilterMode");
                    writer.Int((int)r->minFilterMode());

                    writer.String("dataSize");
                    writer.Int(r->rawDataSize());

                    writer.String("data");
                    writer.String
                    (
                        (const char*)r->rawData(), r->rawDataSize(), false
                    );

                    break;
                }
                case Resource::Type::Cubemap :
                {
                    writer.String("type");
                    writer.String(Resource::typeToName[r->type()].c_str());

                    writer.String("maginificationFilterMode");
                    writer.Int((int)r->magFilterMode());

                    writer.String("minimizationFilterMode");
                    writer.Int((int)r->minFilterMode());

                    writer.String("faces");
                    writer.StartArray();
                    for (int i=0; i<6; i++)
                        writer.String
                        (
                            r->referencedResourcesCRef()[i]->name().c_str()
                        );
                    writer.EndArray();

                    break;
                }
            }
            writer.EndObject();
        }

        writer.EndObject();
    }
};

}

#endif
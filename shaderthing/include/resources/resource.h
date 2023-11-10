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

#ifndef ST_RESOURCE_H
#define ST_RESOURCE_H

#include <string>
#include <vector>
#include <unordered_map>

#include "vir/include/vir.h"

namespace ShaderThing
{

class ObjectIO;

class Resource
{
public:

    enum class Type
    {
        Uninitialized,
        Texture2D,
        FramebufferColorAttachment,
        Cubemap
    };

    static std::unordered_map<Resource::Type, std::string> typeToName;
    static std::unordered_map<std::string, Resource::Type> nameToType;

    static bool validCubemapFace(Resource* face);
    static bool validCubemapFaces(Resource* faces[6]);

private:

    Type type_;
    void* nativeResource_;
    std::vector<Resource*> referencedResources_;
    std::string* namePtr_;
    std::string originalFileExtension_;
    int rawDataSize_;
    unsigned char* rawData_;
    int lastBoundUnit_ = -1;

public:

    // Constructor/destructor
    Resource();
    Resource(const ObjectIO& reader, const std::vector<Resource*>& resources);
    ~Resource();
    Resource& operator=(const Resource&) = delete;

    // Generic public functions
    void reset();
    bool valid() const 
    {
        return nativeResource_ != nullptr || type_ == Type::Uninitialized;
    }
    void bind(int unit);
    void unbind(int unit=-1);

    // Accessors
    Type type() const {return type_;}
    std::string name() const {return *namePtr_;}
    std::string* namePtr() {return namePtr_;}
    const std::string* nameCPtr() const {return namePtr_;}
    std::string originalFileExtension() const {return originalFileExtension_;}
    int id() const;
    int width() const;
    int height() const;
    int rawDataSize() const {return rawDataSize_;}
    const unsigned char* rawData() const {return rawData_;}
    vir::TextureBuffer::WrapMode wrapMode(int index);
    vir::TextureBuffer::FilterMode magFilterMode();
    vir::TextureBuffer::FilterMode minFilterMode();
    const std::vector<Resource*>& referencedResourcesCRef() const 
    {
        return referencedResources_;
    }
    
    // Setters
    template<typename T> bool set(T nativeResource);
    bool set(const unsigned char* data, unsigned int size);
    void setNamePtr(std::string*);
    void setOriginalFileExtension(std::string ofe) {originalFileExtension_=ofe;}
    void setRawData(unsigned char* rawData, int rawDataSize);
    void setWrapMode(int index, vir::TextureBuffer::WrapMode mode);
    void setMagFilterMode(vir::TextureBuffer::FilterMode mode);
    void setMinFilterMode(vir::TextureBuffer::FilterMode mode);

    // Operators
    bool operator==(const Resource& rhs) {return id() == rhs.id();}

    // Serialization
    /*
    template<typename RapidJSONWriterType>
    void saveState(RapidJSONWriterType& writer)
    {
        if
        (
            type_ == Resource::Type::Uninitialized ||
            type_ == Resource::Type::FramebufferColorAttachment
        )
            return;

        writer.String(namePtr_->c_str());
        writer.StartObject();
        switch (type_)
        {
            case Resource::Type::Texture2D :
            {
                writer.String("type");
                writer.String(Resource::typeToName[type_].c_str());

                writer.String("originalFileExtension");
                writer.String(originalFileExtension_.c_str());

                writer.String("wrapModes");
                writer.StartArray();
                for (int i=0; i<2; i++)
                    writer.Int((int)wrapMode(i));
                writer.EndArray();

                writer.String("maginificationFilterMode");
                writer.Int((int)magFilterMode());

                writer.String("minimizationFilterMode");
                writer.Int((int)minFilterMode());

                writer.String("dataSize");
                writer.Int(rawDataSize_);

                writer.String("data");
                writer.String
                (
                    (const char*)rawData_, rawDataSize_, false
                );

                break;
            }
            case Resource::Type::Cubemap :
            {
                writer.String("type");
                writer.String(Resource::typeToName[type_].c_str());

                writer.String("maginificationFilterMode");
                writer.Int((int)magFilterMode());

                writer.String("minimizationFilterMode");
                writer.Int((int)minFilterMode());

                writer.String("faces");
                writer.StartArray();
                for (int i=0; i<6; i++)
                    writer.String
                    (
                        referencedResources_[i]->name().c_str()
                    );
                writer.EndArray();

                break;
            }
        }
        writer.EndObject();
    }
    */

   void saveState(ObjectIO& writer);
};

}

#endif
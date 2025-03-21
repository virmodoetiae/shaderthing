/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2025 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#include <algorithm>
#include <sstream>
#include <string>
#include <vector>

#include "shaderthing/include/objectio.h"

#include "thirdparty/rapidjson/include/rapidjson/document.h"
#include "thirdparty/rapidjson/include/rapidjson/reader.h"
#include "thirdparty/rapidjson/include/rapidjson/writer.h"
#include "thirdparty/rapidjson/include/rapidjson/prettywriter.h"
#include "thirdparty/glm/glm.hpp"

namespace ShaderThing
{

// Typedefs ------------------------------------------------------------------//

typedef rapidjson::PrettyWriter<rapidjson::StringBuffer> nativeWriter;

typedef rapidjson::GenericObject<false, rapidjson::Value> nativeReader;

typedef rapidjson::StringBuffer nativeWriteBuffer;

typedef std::string nativeReadBuffer;

// Static members ------------------------------------------------------------//

// Input file when using ObjectIO in Read mode
std::ifstream ObjectIO::iFile_ = std::ifstream();

// Native data buffer used by rapidson objects for reading/writing data
void* ObjectIO::nativeBuffer_ = nullptr;

// Private methods -----------------------------------------------------------//

ObjectIO::ObjectIO(const char* name, Mode mode, void* nativeObject) :
name_(name),
mode_(mode),
isReadingFromMemory_(false),
isRoot_(false),
isValid_(true),
members_(0),
nativeObject_(nativeObject)
{
    findMembers();
}

void ObjectIO::findMembers()
{
    if (mode_ == Mode::Write || members_.size() > 0)
        return;
    auto reader = (nativeReader*)nativeObject_;
    for (auto m = reader->MemberBegin(); m != reader->MemberEnd(); ++m)
        members_.push_back(m->name.GetString());
}

void ObjectIO::freeNativeMemory()
{
    switch (mode_)
    {
    case Mode::Write :
    {
        if (nativeBuffer_ != nullptr && isRoot_)
            delete (nativeWriteBuffer*) nativeBuffer_;
        if (nativeObject_ != nullptr)
            delete (nativeWriter*) nativeObject_;
        break;
    }
    case Mode::Read :
    {
        if (nativeBuffer_ != nullptr && isRoot_)
            delete (nativeReadBuffer*)nativeBuffer_;
        if (nativeObject_ != nullptr)
            delete (nativeReader*) nativeObject_;
        break;
    }
    }
    if (isRoot_)
        nativeBuffer_ = nullptr;
    nativeObject_ = nullptr;
}

// Public methods ------------------------------------------------------------//

ObjectIO::ObjectIO(const char* filepath, Mode mode) :
name_(filepath),
mode_(mode),
isReadingFromMemory_(false),
isRoot_(true),
isValid_(true),
members_(0),
nativeObject_(nullptr)
{
    switch (mode)
    {
    case Mode::Write :
    {
        nativeBuffer_ = (void*) new nativeWriteBuffer();
        nativeObject_ = (void*) new nativeWriter
        (
            *(nativeWriteBuffer*)nativeBuffer_
        );
        ((nativeWriter*)nativeObject_)->StartObject();
        break;
    }
    case Mode::Read:
    {
        iFile_ = std::ifstream
        (
            filepath, 
            std::ios_base::in | std::ios::binary
        );
        if (!iFile_)
        {
            isValid_ = false;
            return;
        }
        nativeBuffer_ = (void*) new nativeReadBuffer;
        auto* data = (nativeReadBuffer*)nativeBuffer_;
        iFile_.seekg(0, std::ios::end);
        data->resize(iFile_.tellg());
        iFile_.seekg(0, std::ios::beg);
        uint32_t size(data->size());
        iFile_.read(&((*data)[0]), size);
        rapidjson::Document* document = new rapidjson::Document;
        if (document->ParseInsitu(&((*data)[0])).HasParseError())
        {
            freeNativeMemory();
            isValid_ = false;
            return;
        }
        nativeObject_ = (void*) new nativeReader(document->GetObject());
        break;
    }
    }
    findMembers();
}

ObjectIO::ObjectIO(const std::string& json) :
name_(""),
mode_(Mode::Read),
isReadingFromMemory_(true),
isRoot_(true),
isValid_(true),
members_(0),
nativeObject_(nullptr)
{
    nativeBuffer_ = (void*) new nativeReadBuffer;
    auto* data = (nativeReadBuffer*)nativeBuffer_;
    data->resize(json.size());
    std::memcpy(&((*data)[0]), json.c_str(), json.size());
    rapidjson::Document* document = new rapidjson::Document;
    if (document->ParseInsitu(&((*data)[0])).HasParseError())
    {
        freeNativeMemory();
        isValid_ = false;
        return;
    }
    nativeObject_ = (void*) new nativeReader(document->GetObject());
    findMembers();
}

ObjectIO::~ObjectIO()
{
    if (isRoot_ && mode_ == Mode::Read && isReadingFromMemory_)
        iFile_.close();
    freeNativeMemory();
}

bool ObjectIO::writeContentsToDisk() const
{
    if (!isRoot_ || mode_ != Mode::Write)
        return false;
    ((nativeWriter*)nativeObject_)->EndObject();
    std::ostringstream oFileContent;
    try 
    {
        oFileContent << ((nativeWriteBuffer*)nativeBuffer_)->GetString();
    }
    catch (...) 
    {
        return false;
    }
    std::ofstream oFile;
    oFile.open // name_ == filepath for the root object
    (
        name_, 
        std::ios_base::out | std::ios_base::binary
    );
    if(!oFile.is_open())
        return false;
    oFile << oFileContent.str();
    oFile.close();
    return true;
}

bool ObjectIO::hasMember(const char* key) const
{
    return std::find_if
    (
        members_.begin(), 
        members_.end(), 
        [key](const char* mKey)
        {
            return std::strcmp(key, mKey) == 0;
        } 
    ) != members_.end(); 
}

#define ASSERT_READ_MODE_OR_THROW(func_name)                    \
    if (mode_ == Mode::Write)                                   \
        throw std::runtime_error                                \
        (                                                       \
            "ObjectIO::"#func_name" can only be called on "     \
            "read-mode object"                                  \
        );

#define ASSERT_READ_MODE_OR_RETURN(return_obj)                  \
    if (mode_ == Mode::Write)                                   \
        return return_obj;

#define GET_FROM_IOOBJECT(key, type)                            \
     ((nativeReader*)nativeObject_)->operator[](key).Get##type()

#define CHECK_KEY                                               \
    if (!hasMember(key))                                        \
        return defaultValue;

ObjectIO ObjectIO::readObject(const char* key) const
{
    ASSERT_READ_MODE_OR_THROW(readObject(const char* key))
    return ObjectIO
    (
        key, 
        mode_, 
        (void*) new nativeReader(GET_FROM_IOOBJECT(key, Object))
    );
}

template<>
bool ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(false)
    return GET_FROM_IOOBJECT(key, Bool);
}

template<>
bool ObjectIO::readOrDefault(const char* key, bool defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(false)
    CHECK_KEY
    return GET_FROM_IOOBJECT(key, Bool);
}

template<>
int ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(0)
    return GET_FROM_IOOBJECT(key, Int);
}

template<>
int ObjectIO::readOrDefault(const char* key, int defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(0)
    CHECK_KEY
    return GET_FROM_IOOBJECT(key, Int);
}

template<>
unsigned int ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(0)
    return GET_FROM_IOOBJECT(key, Int);
}

template<>
unsigned int ObjectIO::readOrDefault
(
    const char* key, 
    unsigned int defaultValue
) const
{
    ASSERT_READ_MODE_OR_RETURN(0)
    CHECK_KEY
    return GET_FROM_IOOBJECT(key, Int);
}

template<>
float ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(0.0f)
    return GET_FROM_IOOBJECT(key, Double);
}

template<>
float ObjectIO::readOrDefault(const char* key, float defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(0.0f)
    CHECK_KEY
    return GET_FROM_IOOBJECT(key, Double);
}

template<>
double ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(0)
    return GET_FROM_IOOBJECT(key, Double);
}

template<>
double ObjectIO::readOrDefault(const char* key, double defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(0)
    CHECK_KEY
    return GET_FROM_IOOBJECT(key, Double);
}

#define GET_ARRAY(dim, type, glmtype)                                       \
    glmtype##dim v;                                                         \
    auto a = ((nativeReader*) nativeObject_)->operator[](key).GetArray();   \
    for (int i=0; i<dim; i++)                                               \
        v[i] = a[i].Get##type();                                            \
    return v;

template<>
glm::ivec2 ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::ivec2(0))
    GET_ARRAY(2, Int, glm::ivec)
}

template<>
glm::ivec2 ObjectIO::readOrDefault
(
    const char* key, 
    glm::ivec2 defaultValue
) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::ivec2(0))
    CHECK_KEY
    GET_ARRAY(2, Int, glm::ivec)
}

template<>
glm::ivec3 ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::ivec3(0))
    GET_ARRAY(3, Int, glm::ivec)
}

template<>
glm::ivec3 ObjectIO::readOrDefault(const char* key, glm::ivec3 defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::ivec3(0))
    CHECK_KEY
    GET_ARRAY(3, Int, glm::ivec)
}

template<>
glm::ivec4 ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::ivec4(0))
    GET_ARRAY(4, Int, glm::ivec)
}

template<>
glm::ivec4 ObjectIO::readOrDefault(const char* key, glm::ivec4 defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::ivec4(0))
    CHECK_KEY
    GET_ARRAY(4, Int, glm::ivec)
}

template<>
glm::vec2 ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::vec2(0))
    GET_ARRAY(2, Double, glm::vec)
}

template<>
glm::vec2 ObjectIO::readOrDefault(const char* key, glm::vec2 defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::vec2(0))
    CHECK_KEY
    GET_ARRAY(2, Double, glm::vec)
}

template<>
glm::vec3 ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::vec3(0))
    GET_ARRAY(3, Double, glm::vec)
}

template<>
glm::vec3 ObjectIO::readOrDefault(const char* key, glm::vec3 defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::vec3(0))
    CHECK_KEY
    GET_ARRAY(3, Double, glm::vec)
}

template<>
glm::vec4 ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::vec4(0))
    GET_ARRAY(4, Double, glm::vec)
}

template<>
glm::vec4 ObjectIO::readOrDefault(const char* key, glm::vec4 defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(glm::vec4(0))
    CHECK_KEY
    GET_ARRAY(4, Double, glm::vec)
}

template<>
std::string ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(std::string())
    return std::string(GET_FROM_IOOBJECT(key, String));
}

template<>
std::string ObjectIO::readOrDefault(const char* key, std::string defaultValue) const
{
    ASSERT_READ_MODE_OR_RETURN(std::string())
    CHECK_KEY
    return std::string(GET_FROM_IOOBJECT(key, String));
}

template<>
std::vector<std::string> ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(std::vector<std::string>(0))
    auto a = ((nativeReader*)nativeObject_)->operator[](key).GetArray();
    std::vector<std::string> v(a.Size());
    for (int i=0; i<(int)a.Size(); i++)
    {
        v[i] = a[i].GetString();
    }
    return v;
}

template<>
std::vector<const char*> ObjectIO::read(const char* key) const
{
    ASSERT_READ_MODE_OR_RETURN(std::vector<const char*>(0))
    auto a = ((nativeReader*)nativeObject_)->operator[](key).GetArray();
    std::vector<const char*> v(a.Size());
    for (int i=0; i<(int)a.Size(); i++)
    {
        // Force copy
        unsigned int srcSize = a[i].GetStringLength();
        const char* src = a[i].GetString();
        auto dst = new char [srcSize];
        memcpy((void*)dst, src, srcSize);
        v[i] = dst;
    }
    return v;
}

template<>
std::vector<const char*> ObjectIO::readOrDefault
(
    const char* key, 
    std::vector<const char*> defaultValue
) const
{
    ASSERT_READ_MODE_OR_RETURN(std::vector<const char*>(0))
    CHECK_KEY
    auto a = ((nativeReader*)nativeObject_)->operator[](key).GetArray();
    std::vector<const char*> v(a.Size());
    for (int i=0; i<(int)a.Size(); i++)
    {
        // Force copy
        unsigned int srcSize = a[i].GetStringLength();
        const char* src = a[i].GetString();
        auto dst = new char [srcSize];
        memcpy((void*)dst, src, srcSize);
        v[i] = dst;
    }
    return v;
}

const char* ObjectIO::read(const char* key, bool copy, unsigned int* size) const
{
    ASSERT_READ_MODE_OR_RETURN(nullptr)
    if (!hasMember(key))
        return nullptr;
    auto data = GET_FROM_IOOBJECT(key, String);
    if (copy)
    {   
        unsigned int dsize = GET_FROM_IOOBJECT(key, StringLength);
        if (size != nullptr)
            *size = dsize;
        char* cdata = new char[dsize];
        memcpy(cdata, data, dsize);
        return cdata;
    }
    else if (size != nullptr)
        *size = GET_FROM_IOOBJECT(key, StringLength);
    return data;
}

#define ASSERT_WRITE_MODE_OR_THROW(func_name)                   \
    if (mode_ == Mode::Read)                                    \
        throw std::runtime_error                                \
        (                                                       \
            "ObjectIO::"#func_name" can only be called on "     \
            "write-mode objects"                                \
        );

#define ASSERT_WRITE_MODE_OR_RETURN                             \
    if (mode_ == Mode::Read)                                    \
        return;

#define WRITE_VALUE(type)                                       \
    auto* writer=(nativeWriter*) nativeObject_;                 \
    writer->String(key);                                        \
    writer->type(value);

#define WRITE_ARRAY(dim, type)                                  \
    auto* writer = (nativeWriter*) nativeObject_;               \
    writer->String(key);                                        \
    writer->StartArray();                                       \
    for (int i=0; i<dim; i++)                                   \
        writer->type(value[i]);                                 \
    writer->EndArray();

template<>
void ObjectIO::write(const char* key, const bool& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_VALUE(Bool)
}

template<>
void ObjectIO::write(const char* key, const int& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_VALUE(Int)
}

template<>
void ObjectIO::write(const char* key, const unsigned int& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_VALUE(Int)
}

template<>
void ObjectIO::write(const char* key, const float& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_VALUE(Double)
}

template<>
void ObjectIO::write(const char* key, const double& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_VALUE(Double)
}

template<>
void ObjectIO::write(const char* key, const glm::ivec2& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_ARRAY(2, Int)
}

template<>
void ObjectIO::write(const char* key, const glm::ivec3& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_ARRAY(3, Int)
}

template<>
void ObjectIO::write(const char* key, const glm::ivec4& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_ARRAY(4, Int)
}

template<>
void ObjectIO::write(const char* key, const glm::vec2& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_ARRAY(2, Double)
}

template<>
void ObjectIO::write(const char* key, const glm::vec3& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_ARRAY(3, Double)
}

template<>
void ObjectIO::write(const char* key, const glm::vec4& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    WRITE_ARRAY(4, Double)
}

template<>
void ObjectIO::write(const char* key,const std::vector<std::string>& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    auto* writer = (nativeWriter*)nativeObject_;
    writer->String(key);
    writer->StartArray();
    for (auto vi : value)
        writer->String(vi.c_str(), vi.size());
    writer->EndArray();
}

template<>
void ObjectIO::write(const char* key,const std::vector<const char*>& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    auto* writer = (nativeWriter*)nativeObject_;
    writer->String(key);
    writer->StartArray();
    for (auto vi : value)
        writer->String(vi);
    writer->EndArray();
}

template<>
void ObjectIO::write(const char* key, const std::string& value)
{
    ASSERT_WRITE_MODE_OR_RETURN
    write(key, value.c_str(), value.size());
}

void ObjectIO::write
(
    const char* key, 
    const char* value, 
    unsigned int size,
    bool writeSize
)
{
    ASSERT_WRITE_MODE_OR_RETURN
    auto* writer = (nativeWriter*)nativeObject_;
    if (writeSize && size > 0)
    {
        std::string sizeKey(key);
        sizeKey += "Size";
        writer->String(sizeKey.c_str());
        writer->Int(size);
    }
    writer->String(key);
    size > 0 ? writer->String(value, size, false) : writer->String(value);
}

void ObjectIO::writeObjectStart(const char* key)
{
    ASSERT_WRITE_MODE_OR_RETURN
    auto* writer = (nativeWriter*)nativeObject_;
    writer->String(key);
    writer->StartObject();
}

void ObjectIO::writeObjectEnd()
{
    ASSERT_WRITE_MODE_OR_RETURN
    auto* writer = (nativeWriter*)nativeObject_;
    writer->EndObject();
}

}
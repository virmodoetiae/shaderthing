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

#ifndef ST_OBJECT_IO_H
#define ST_OBJECT_IO_H

#include <fstream>
#include <unordered_map>

namespace ShaderThing
{

// Wrapper class for rapidjson-based IO. I won't lie, most of this comes from my
// inability to figure out a way to forward-declare the very-template-heavy
// rapidjson classes in other header files, and my unwillingness to just include
// the rapidson headers directly in my headers
class ObjectIO
{
public:

    enum class Mode
    {
        Read,
        Write
    };

protected:

    // Output file when using ObjectIO in Write mode
    static std::ofstream oFile_;

    // Input file when using ObjectIO in Read mode
    static std::ifstream iFile_;

    // Native data buffer used by rapidson objects for reading/writing data
    static void* nativeBuffer_;
    
    // If this object is the root one, 'name_' consists of the input/output
    // file path. Otherwise, it consists of the object key name
    const char* name_;

    // Mode (Read or Write)
    Mode mode_;

    // True for root objects (i.e., user-created ones)
    bool isRoot_;

    // Native rapidjson object for reading/writing data
    void* nativeObject_;

    //
    ObjectIO(const char* name, Mode mode, void* nativeObject);

    //
    void freeNativeMemory();
    
public:

    // Construct from input/output filepath and mode (either Read or Write)
    ObjectIO(const char* filepath, Mode mode);
    
    // 
    ~ObjectIO();

    //
    ObjectIO getObject(const char* key);

    //
    template<typename T>
    T getValue(const char* key);

    //
    const char* getValue(const char* key, bool copy, unsigned int* size=nullptr);

    //
    template<typename T>
    void writeValue(const char* key, const T& value);

    //
    void writeValue(const char* key, const char* value, unsigned int size=0);
    
    //
    void writeObjectStart(const char* key);
    
    //
    void writeObjectEnd();
};

}

#endif
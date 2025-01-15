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

#ifndef ST_OBJECT_IO_H
#define ST_OBJECT_IO_H

#include <fstream>
#include <vector>

namespace ShaderThing
{

// Wrapper class for rapidjson-based IO. I won't lie, most of this comes from my
// inability to figure out a way to forward-declare the very-template-heavy
// rapidjson classes in other header files, and my unwillingness to just include
// the rapidson headers directly in my headers. Nonetheless, I feel this is
// ultimately a cleaner solution from a higher-level-code perspective
class ObjectIO
{
public:

    enum class Mode
    {
        Read,
        Write
    };

protected:

    // Input file when using ObjectIO in Read mode
    static std::ifstream iFile_;

    // Native data buffer used by rapidson objects for reading/writing data
    static void* nativeBuffer_;
    
    // If this object is the root one, 'name_' consists of the input/output
    // file path. Otherwise, it consists of the object key name
    const char* name_;

    // Mode (Read or Write)
    Mode mode_;

    //
    bool isReadingFromMemory_;

    // True for root objects (i.e., user-created ones)
    bool isRoot_;

    // False if:
    // - failed to open a file for reading and/or writing
    // - failed to read from an opened file
    // - failed to write to an opened file
    // The underlying files are never modified if such issues arise
    bool isValid_;

    // List of member names within this object
    std::vector<const char*> members_;

    // Native rapidjson object for reading/writing data
    void* nativeObject_;

    //
    ObjectIO(const char* name, Mode mode, void* nativeObject);

    // Determines the list of members for read-only objects. Run at 
    // initialization
    void findMembers();

    //
    void freeNativeMemory();
    
public:

    // Construct from in-memory JSON (Read-only mode)
    ObjectIO(const std::string& json);

    // Construct from input/output filepath and mode (either Read or Write)
    ObjectIO(const char* filepath, Mode mode);
    
    // Destroy, which writes data to the output file if in write mode and closes
    // the i/o file depending on the mode
    ~ObjectIO();

    // Get name. In write mode, or if in read mode and this object is root, this
    // consists of the i/o file path. If in read mode and this object is not
    // root, it consists of the JSON sub-dict key name
    const char* name() const {return name_;}

    // False if:
    // - failed to open a file for reading and/or writing on construction
    // - failed to read from an opened file on construction
    //
    // The underlying files are never modified if such issues arise
    bool isValid() const {return isValid_;}

    // If this ObjectIO object was created in write mode and if it is the root 
    // one, save its contents to disk, namely to the filepath specified at 
    // ObjectIO construction. Returns true on write success. Returns false if
    // the mentioned preconditions are not met, if the ObjectIO contents are
    // corrupted or if the output file cannot be opened for writing. In the
    // case of corrupted data, the output file contents are not ovewritten and
    // nothing is lost
    bool writeContentsToDisk() const;

    // Get all JSON member names (i.e., keys) in this object
    const std::vector<const char*>& members() const {return members_;}

    // True if this object contains the provided member/key name
    bool hasMember(const char* key) const;

    // If in read mode, returns a new object representing the JSON sub-dict of
    // the provided key/member name. If in write mode, or if the provided key
    // is not found as this object's member, it throws a runtime exception.
    // Use with together with 'hasMember(const char* key)' to avoid such 
    // situations
    ObjectIO readObject(const char* key) const;

    // Read a value of type T under the provided key/member entry. If the 
    // provided key/member name does not exist, an exception is thrown
    template<typename T>
    T read(const char* key) const;

    // Read a value of type T under the provided key/member entry. If the 
    // provided key/member name does not exist, the provided defaultValue
    // is returned
    template<typename T>
    T readOrDefault(const char* key, T defaultValue) const;

    // Read a const char* under the provided key/member entry. If 'copy' is
    // true, the returned const char* will be a copy of the locally cached const
    // char* in native JSON-reader memory. If not, the content of the
    // const char* will go out of scope after top-level (root) object
    // destruction. The size of the read const char* data may also be retrieved
    // via passing a 'size' pointer. This is important as the retruned
    // const char* is not necessarily null-terminated. If the key is not found
    // a nullptr is returned
    const char* read
    (
        const char* key, 
        bool copy, 
        unsigned int* size=nullptr
    ) const;

    // Write a value of type under the provided key/member name
    template<typename T>
    void write(const char* key, const T& value);

    // Write a const char* under the provided key/member name. To make the 
    // function more efficient, the size of the const char* value to be written
    // can be provided. If so, an optional additional key with the size of the
    // written value may be written if writeSize is set to true. Its key name
    // will be key+'Size'
    void write
    (
        const char* key, 
        const char* value, 
        unsigned int size=0, 
        bool writeSize=false
    );
    
    // If in write mode, signals that all further write calls will write inside
    // an object (JSON sub-dictionary) of name 'key'
    void writeObjectStart(const char* key);
    
    // If in write mode, signals the end of the object (JSON sub-dictionary)
    // whose start was previous signalled with 
    // 'writeObjectStart(const char* key)'
    void writeObjectEnd();
};

}

#endif
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

    // Resource types tied to the undelying vir:: graphics buffers
    enum class Type
    {
        Uninitialized,
        Texture2D,
        AnimatedTexture2D,
        FramebufferColorAttachment,
        Cubemap
    };

    // Static mappings for convenience
    static std::unordered_map<Resource::Type, std::string> typeToName;
    static std::unordered_map<std::string, Resource::Type> nameToType;

    // Checks if the provided resource would be a valid face for a cubemap.
    // The key aspects are: 1) being of Texture2D type; 2) having a square
    // aspect ratio; 3) having a resolution which is a multiple of 2 along
    // both axes
    static bool validCubemapFace(Resource* face);

    // Checks if the provided array of resources would be a valid set of
    // faces for a new cubemap resource
    static bool validCubemapFaces(Resource* faces[6]);

    // Struct for wrapping additional data in case this resource is
    // of AnimatedTexture2D type
    struct AnimationData
    {
        float internalTime = 0.0;
        bool isInternalTimePaused = false;
    };

private:

    // Resource type
    Type type_;

    // void pointer to native underlying vir:: graphics buffer, i.e., the
    // internally-managed resource
    void* nativeResource_;

    // List of resources consisting of: 1) resources acting as cubemap faces
    // when this resource is of Cubemap type; OR 2) resources acting as 
    // animation frames when this resource is of AnimatedTexture2D type. Please
    // note that in both cases the pointed resources will be of Texture2D type
    // (not dynamic resources, e.g., buffer, supported for now)
    std::vector<Resource*> referencedResources_;

    // Name of this resource as a pointer
    std::string* namePtr_;

    // File extensions of the original file used for loading the nativeResource_
    // of this resource (e.g., .png, .gif, etc.). This is empty for internally
    // generated resources (e.g., Cubemap)
    std::string originalFileExtension_;

    // Size of the raw data used for loading the nativeResource_ (0 for 
    // internally managed resources)
    int rawDataSize_;

    // Actual raw data used for loading the nativeResource_ (nullptr for
    // internally managed resources). The point of storing raw data instead
    // of discarding it after nativeResource_ loading is to be able to
    // re-save the raw data within a ShaderThing project file (.stf) to
    // enable re-loading the resource on project loading
    unsigned char* rawData_;

    // Form an OpenGL perspective, this is the OpenGL texture unit to which
    // this resource was bound last. Nonetheless, the concept of texture
    // units is not specific to OpenGL, so there is no loss of generaly if
    // e.g., the vir:: back-end were to now support and use Vulkan (which, in 
    // all honestly, it will probably never do, although I tried to structure it
    // it in a sensible way)
    int lastBoundUnit_ = -1;

    // Specifically for AnimatedTexture2Ds
    AnimationData* animationData_;

public:

    // Constructor/destructor
    Resource();
    Resource(const ObjectIO& reader, const std::vector<Resource*>& resources);
    ~Resource();
    Resource& operator=(const Resource&) = delete;

    // Reset all resource members and delete the native resource
    void reset();

    // If this resource is of AnimatedTexture2D type, this will advance the 
    // animation in time according by the provided time step. Otherwise, this
    // function has no effect
    void update(float dt);

    // True if this resource is actually managing a nativeResource of any kind
    bool valid() const 
    {
        return nativeResource_ != nullptr || type_ == Type::Uninitialized;
    }

    // Bind the managed resource to a provided graphics card texture unit
    void bind(int unit);

    // Unbind the managed resource
    void unbind(int unit=-1);

    // Serialize the state of this Resource to the provided ObjectIO writer
    // (a JSON-like data structure for disk I/O operations)
    void saveState(ObjectIO& writer);

    // Internal time play/pause toggle if this resource is an AnimatedTexture2D,
    // no effect otherwise
    void toggleAnimationPaused();

    // Steps the animation to the next frame, regardless of whether the internal
    // animation time is paused or not. Has an effect only if this resource is
    // an AnimatedTexture2D
    void advanceAnimationFrame();

    // Accessors -------------------------------------------------------------//

    // Resource type
    Type type() const {return type_;}

    // Resource name
    std::string name() const {return *namePtr_;}

    // Modifiable pointer to resource name
    std::string* namePtr() {return namePtr_;}

    // Unmodifiable pointer to resource name
    const std::string* nameCPtr() const {return namePtr_;}

    // Original file extension
    std::string originalFileExtension() const {return originalFileExtension_;}

    // Id of the internally managed resource. If contentId is true and this
    // resource is of AnimatedTexture2D type, it will return the id of the
    // current frame managed by the internally managed resource
    int id(bool contentId=true) const;

    // Width of the internally managed resource. If this resource
    // is a Cubemap, it returns the width of any of its faces, while if it is
    // an AnimatedTexture2D, it returns the width of any of its frames
    int width() const;

    // Height of the internally managed resource. If this resource
    // is a Cubemap, it returns the height of any of its faces, while if it is
    // an AnimatedTexture2D, it returns the height of any of its frames
    int height() const;

    // Size of the raw data which was used for loading the internally-managed
    // resource, if any
    int rawDataSize() const {return rawDataSize_;}

    // Unmodified pointer to the raw data which was used for loading the
    // internally-managed resource, if any
    const unsigned char* rawData() const {return rawData_;}

    // Wrap mode of the internally-managed resource along direction the 'width'
    // direction (if index == 0) or 'height' direction (if index == 1)
    vir::TextureBuffer::WrapMode wrapMode(int index);

    // Interpolation mode of the internally-managed resource when magnified
    vir::TextureBuffer::FilterMode magFilterMode();

    // Interpolation mode of the internally-managed resource when minimized
    vir::TextureBuffer::FilterMode minFilterMode();

    // Ref to referenced resources, if any. Can consist of either a list of
    // other resources used as Cubemap faces if this resource is a Cubemap, or
    // a list of other resources used animation frames if this resource is an
    // AnimatedTexture2D
    const std::vector<Resource*>& referencedResourcesCRef() const 
    {
        return referencedResources_;
    }

    // True if this resource is an AnimatedTexture2D and the internal animation
    // time is paused
    bool isAnimationPaused() const;

    // Animation speed in frames per second if this resource is an 
    // AnimatedTexture2D, returns 0 otherwise
    float animationFps() const;
    
    // Setters ---------------------------------------------------------------//
    
    // Initialize the internally-managed resource depending on the nature of the
    // passed data
    template<typename T> bool set(T nativeResource);

    // Initialize the internally-managed resource from the provided raw data and
    // its size
    bool set(const unsigned char* data, unsigned int size);

    // Set the name pointer to the provided one (ownership is transferred to 
    // this resource)
    void setNamePtr(std::string*);

    // Set the original filename extension
    void setOriginalFileExtension(std::string ofe) {originalFileExtension_=ofe;}

    // Set the raw data
    void setRawData(unsigned char* rawData, int rawDataSize);

    // Set the wrap mode of the internally-managed resource to the provided one
    // in the given direction index (horizontal if 0, vertical if 1)
    void setWrapMode(int index, vir::TextureBuffer::WrapMode mode);

    // Set the interpolation mode of the internally-managed resource on 
    // magnification to the provided one
    void setMagFilterMode(vir::TextureBuffer::FilterMode mode);

    // Set the interpolation mode of the internally-managed resource on 
    // minimization to the provided one
    void setMinFilterMode(vir::TextureBuffer::FilterMode mode);

    // Set the animation speed in frames per second. Has an effect only if this
    // resource is an AnimatedTexture2D
    void setAnimationFps(float fps);

    // Operators -------------------------------------------------------------//

    bool operator==(const Resource& rhs) {return id(false) == rhs.id(false);}

};

}

#endif
#include "shaderthing-p/include/resource.h"
#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/helpers.h"
#include "shaderthing-p/include/filedialog.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/stb/stb_image.h"

namespace ShaderThing
{

FileDialog      Resource::fileDialog_;
const Resource* Resource::resourceToBeExported_ = nullptr;
Resource**      Resource::resourceToBeReplaced_ = nullptr;

void Resource::update(std::vector<Resource*>& resources, const UpdateArgs& args)
{
    for (auto* resource : resources)
        resource->update(args);
    if (!Resource::fileDialog_.validSelection())
        return;
    if (resourceToBeExported_ != nullptr) // Export
    {
        auto resource = Resource::resourceToBeExported_;
        auto filepath = fileDialog_.selection().front();
        const unsigned char* rawData = 
            resource->type_ == Resource::Type::Texture2D ?
            ((const Texture2DResource*)resource)->rawData_ :
            ((const AnimatedTexture2DResource*)resource)->rawData_;
        unsigned int rawDataSize = 
            resource->type_ == Resource::Type::Texture2D ?
            ((const Texture2DResource*)resource)->rawDataSize_ :
            ((const AnimatedTexture2DResource*)resource)->rawDataSize_;
        std::ofstream file;
        file.open(filepath, std::ios_base::out|std::ios_base::binary);
        if(file.is_open())
        {
            file.write((const char*)rawData, rawDataSize);
            file.close();
        }
        Resource::resourceToBeExported_ = nullptr;
    }
    else if (Resource::resourceToBeReplaced_ != nullptr)
    {
        Resource*& resource = *Resource::resourceToBeReplaced_;
        auto name = resource->name();
        auto filepath = fileDialog_.selection().front();
        auto newResource = Resource::create(filepath);
        if (newResource != nullptr)
        {
            if (resource != nullptr)
                delete resource;
            resource = newResource;
            resource->setName(Helpers::filename(filepath));
        }
        Resource::resourceToBeReplaced_ = nullptr;
    }
    else
        for (auto filepath : fileDialog_.selection()) // Load new
        {
            auto newResource = Resource::create(filepath);
            if (newResource != nullptr)
            {
                newResource->setName(Helpers::filename(filepath));
                resources.emplace_back(newResource);
            }
        }
    fileDialog_.clearSelection();
}

Resource::~Resource()
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
}

Resource* Resource::create(const std::string& filepath)
{
    std::string fileExtension = Helpers::fileExtension(filepath);
    if (fileExtension != ".gif")
    {
        auto resource = new Texture2DResource();
        if (resource->set(filepath))
            return resource;
        delete resource;
    }
    else
    {
        auto resource = new AnimatedTexture2DResource();
        if (resource->set(filepath))
            return resource;
        delete resource;
    }
    return nullptr;
}

Resource* Resource::create(unsigned char* rawData, unsigned int size, bool gif)
{
    if (!gif)
    {
        auto resource = new Texture2DResource();
        if (resource->set(rawData, size))
            return resource;
        delete resource;
    }
    else
    {
        auto resource = new AnimatedTexture2DResource();
        if (resource->set(rawData, size))
            return resource;
        delete resource;
    }
    return nullptr;
}

Resource* Resource::create(const std::vector<Texture2DResource*>& frames)
{
    auto resource = new AnimatedTexture2DResource();
    if (resource->set(frames))
        return resource;
    delete resource;
    return nullptr;
}

Resource* Resource::create(const Texture2DResource* faces[6])
{
    auto resource = new CubemapResource();
    if (resource->set(faces))
        return resource;
    delete resource;
    return nullptr;
}

Resource* Resource::create(vir::Framebuffer** framebuffer)
{
    auto resource = new FramebufferResource();
    if (resource->set(framebuffer))
        return resource;
    delete resource;
    return nullptr;
}

void Resource::setName(const std::string& name)
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
    namePtr_ = new std::string(name);
}

void Resource::setNamePtr(std::string* namePtr)
{
    if (namePtr_ != nullptr && isNameManaged_)
        delete namePtr_;
    namePtr_ = namePtr;
}

//----------------------------------------------------------------------------//

#define SET_NATIVE_AND_RAW_AND_RETURN(data, size)                           \
    if (native_ != nullptr) delete native_;                                 \
    native_ = native;                                                       \
    if (rawData_ != nullptr) delete[] rawData_;                             \
    rawData_ = data;                                                        \
    rawDataSize_ = size;                                                    \
    return true;

bool Texture2DResource::set(const std::string& filepath)
{
    unsigned int rawDataSize;
    unsigned char* rawData = Helpers::readFileContents(filepath, rawDataSize);
    vir::TextureBuffer2D* native = nullptr;
    try
    {
        native = vir::TextureBuffer2D::create
        (
            filepath, 
            vir::TextureBuffer::InternalFormat::RGBA_UNI_8
        );
        if (native == nullptr)
        {
            if (rawData != nullptr) delete[] rawData;
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cout 
            << "Texture2DResource::set(const std::string& filepath): "
            << e.what() << std::endl;
        if (rawData != nullptr) delete[] rawData;
        return false;
    }
    originalFileExtension_ = Helpers::fileExtension(filepath);
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, rawDataSize)
}

bool Texture2DResource::set(unsigned char* rawData, unsigned int size)
{
    int width, height, nChannels;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load_from_memory
    (
        rawData, 
        size, 
        &width, 
        &height, 
        &nChannels,
        4 // Force all textures to be treated as 4-component
    );
    nChannels = 4; // Force all textures to be treated as 4-component
    vir::TextureBuffer2D* native = nullptr;
    try
    {
        native = vir::TextureBuffer2D::create
        (
            data, 
            width,
            height,
            vir::TextureBuffer::defaultInternalFormat(nChannels)
        );
        if (native == nullptr)
        {
            stbi_image_free(data);
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cout 
            << "Texture2DResource::set(const unsigned char* rawData, unsigned int size): "
            << e.what() << std::endl;
        stbi_image_free(data);
        return false;
    }
    stbi_image_free(data);
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, size)
}

Texture2DResource::~Texture2DResource()
{
    if (native_ != nullptr)
    {
        this->unbind();
        delete native_;
    }
    if (rawData_ != nullptr) 
        delete[] rawData_;
}

//----------------------------------------------------------------------------//

bool AnimatedTexture2DResource::set(const std::string& filepath)
{
    // Load image data
    unsigned int rawDataSize;
    unsigned char* rawData = Helpers::readFileContents(filepath, rawDataSize);

    // Create native resource from raw data
    vir::AnimatedTextureBuffer2D* native = nullptr;
    try
    {
        native = vir::AnimatedTextureBuffer2D::create
        (
            filepath, 
            vir::TextureBuffer::InternalFormat::RGBA_UNI_8
        );
        if (native == nullptr)
        {
            delete[] rawData;
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cout 
            << "AnimatedTexture2DResource::set(const std::string& filepath): "
            << e.what() << std::endl;
        delete[] rawData;
        return false;
    }

    // Set original file extension and raw data if all went well
    originalFileExtension_ = Helpers::fileExtension(filepath);
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, rawDataSize)
}

bool AnimatedTexture2DResource::set(unsigned char* rawData, unsigned int size)
{
    int width, height, nFrames, nChannels;
    int* frameDuration;
    stbi_set_flip_vertically_on_load(true);
    unsigned char* data = stbi_load_gif_from_memory
    (
        rawData, 
        size, 
        &frameDuration,
        &width, 
        &height, 
        &nFrames,
        &nChannels,
        4 // Force all textures to be treated as 4-component
    );
    nChannels = 4; // Force all textures to be treated as 4-component
    vir::AnimatedTextureBuffer2D* native = nullptr;
    try
    {
        auto native = vir::AnimatedTextureBuffer2D::create
        (
            data, 
            width, 
            height, 
            nFrames,
            vir::TextureBuffer::defaultInternalFormat(nChannels)
        );
        if (native == nullptr)
        {
            stbi_image_free(frameDuration);
            stbi_image_free(data);
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cout 
            << "AnimatedTexture2DResource::set(unsigned char* rawData, unsigned int size): "
            << e.what() << std::endl;
        stbi_image_free(frameDuration);
        stbi_image_free(data);
        return false;
    }
    float duration = 0.f;
    for (int i=0; i<nFrames; i++)
        duration += frameDuration[i];
    native->setFrameDuration(duration/nFrames);
    stbi_image_free(frameDuration);
    stbi_image_free(data);
    SET_NATIVE_AND_RAW_AND_RETURN(rawData, size)
}

bool AnimatedTexture2DResource::set(const std::vector<Texture2DResource*>& frames)
{
    std::vector<vir::TextureBuffer2D*> nativeFrames(frames.size());
    for(int i=0; i<frames.size(); i++)
        nativeFrames[i] = frames[i]->native_;
    vir::AnimatedTextureBuffer2D* native = nullptr;
    try
    {
        native = vir::AnimatedTextureBuffer2D::create
        (
            nativeFrames,
            false
        );
        if (native == nullptr)
            return false;
    }
    catch(const std::exception& e)
    {
        std::cout 
            << "AnimatedTexture2DResource::set(const std::vector<Texture2DResource*>& frames): "
            << e.what() << std::endl;
        return false;
    }
    unmanagedFrames_.clear();
    unmanagedFrames_.resize(frames.size());
    for(int i=0; i<frames.size(); i++)
        unmanagedFrames_[i] = frames[i];
    SET_NATIVE_AND_RAW_AND_RETURN(nullptr, 0)
}

AnimatedTexture2DResource::~AnimatedTexture2DResource()
{
    if (native_ != nullptr)
    {
        this->unbind();
        delete native_;
    }
    if (rawData_ != nullptr)
        delete[] rawData_;
}

void AnimatedTexture2DResource::update(const UpdateArgs& args)
{
    if (isAnimationBoundToGlobalTime_)
        native_->setTime(args.time);
    else if (!isAnimationPaused_)
        native_->advanceTime(args.timeStep);
}

//----------------------------------------------------------------------------//

bool CubemapResource::set(const Texture2DResource* faces[6])
{
    const vir::TextureBuffer2D* nativeFaces[6];
    for (int i=0; i<6; i++)
        nativeFaces[i] = (faces[i])->native_;
    if (!vir::CubeMapBuffer::validFaces(nativeFaces))
        return false;
    
    // Read face data
    const unsigned char* faceData[6];
    stbi_set_flip_vertically_on_load(false);
    int width, height, nChannels;
    for (int i=0; i<6; i++)
    {
        const unsigned char* rawData = faces[i]->rawData_;
        int rawDataSize = faces[i]->rawDataSize_;
        faceData[i] = stbi_load_from_memory
        (
            rawData, 
            rawDataSize, 
            &width, 
            &height, 
            &nChannels,
            4 // Force all textures to be treated as 4-component
        );
    }
    vir::CubeMapBuffer* native;
    try
    {
        native = vir::CubeMapBuffer::create
        (
            faceData, 
            width, 
            height, 
            vir::TextureBuffer::InternalFormat::RGBA_UNI_8
        );
        if (native == nullptr)
        {
            for (int i=0; i<6; i++)
                stbi_image_free((void*)faceData[i]);
            return false;
        }
    }
    catch(const std::exception& e)
    {
        std::cout 
            << "CubemapResource::set(const Texture2DResource* faces[6]): "
            << e.what() << std::endl;
        for (int i=0; i<6; i++)
            stbi_image_free((void*)faceData[i]);
        return false;
    }

    if (native_ != nullptr) delete native_;
    native_ = native;
    for (int i=0; i<6; i++)
    {
        unmanagedFaces_[i] = faces[i];
        stbi_image_free((void*)faceData[i]);
    }
    return true;
}

CubemapResource::~CubemapResource()
{
    if (native_ != nullptr)
    {
        this->unbind();
        delete native_;
    }
}

//----------------------------------------------------------------------------//

bool FramebufferResource::set(vir::Framebuffer** framebuffer)
{
    if (framebuffer == nullptr)
        return false;
    native_ = framebuffer;
    return true;
}

FramebufferResource::~FramebufferResource()
{
    if (native_ != nullptr)
        this->unbind();
}

//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//
//----------------------------------------------------------------------------//

bool Resource::isGuiOpen = false;
bool Resource::isGuiDetachedFromMenu = false;

void Resource::renderResourcesGUI
(
    std::vector<Resource*>& resources,
    const std::vector<Layer*>& layers
)
{
    if (!Resource::isGuiOpen)
        return;
    if (Resource::isGuiDetachedFromMenu)
    {
        ImGui::SetNextWindowSize(ImVec2(900,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags(ImGuiWindowFlags_NoCollapse);
        ImGui::Begin("Resource manager", &Resource::isGuiOpen, windowFlags);
    }

    const float fontSize = ImGui::GetFontSize();
    const float buttonWidth(10*fontSize);
    const float cursorPosY0 = ImGui::GetCursorPosY();

    #define START_ROW                                                       \
        ImGui::PushID(row);                                                 \
        ImGui::TableNextRow(0, 1.6*fontSize);                               \
        column = 0;
    #define END_ROW                                                         \
        ImGui::PopID();
    #define START_COLUMN                                                    \
        ImGui::TableSetColumnIndex(column);                                 \
        ImGui::PushItemWidth(-1);
    #define END_COLUMN                                                      \
        ++column;                                                           \
        ImGui::PopItemWidth();
    #define NEXT_COLUMN                                                     \
        ImGui::TableSetColumnIndex(column++);

    //--------------------------------------------------------------------------
    auto renderResourceGUI = 
    [
        &fontSize, 
        &buttonWidth
    ]
    (
        bool& deleteResource,
        std::vector<Resource*>& resources,
        const int row
    )
    {
        int column = 0;
        START_ROW
        START_COLUMN // Actions column -----------------------------------------
        renderResourceActionsButtonGUI
        (
            resources[row], 
            deleteResource, 
            resources, 
            ImVec2(buttonWidth, 0)
        );
        Resource* resource = resources[row];
        END_COLUMN
        START_COLUMN // Type column --------------------------------------------
        static std::map<Resource::Type, const char*> typeToName
        {
            {Resource::Type::Texture2D,         "Texture-2D"},
            {Resource::Type::AnimatedTexture2D, "Animation-2D"},
            {Resource::Type::Cubemap,           "Cubemap"},
            {Resource::Type::Framebuffer,       "Layer"}
        };
        ImGui::Text(typeToName.at(resource->type_));
        END_COLUMN
        START_COLUMN // Preview column -----------------------------------------
        float x = resource->width();
        float y = resource->height();
        float aspectRatio = x/y;
        auto previewTexture2D = 
        [&aspectRatio]
        (
            const Resource* resource, 
            float sideSize, 
            float offset
        )->void
        {
            ImVec2 previewSize;
            ImVec2 hoverSize{256,256};
            if (aspectRatio > 1.0)
            {
                previewSize = ImVec2(sideSize, sideSize/aspectRatio);
                hoverSize.y /= aspectRatio;
            }
            else
            { 
                previewSize = ImVec2(sideSize*aspectRatio, sideSize);
                hoverSize.x *= aspectRatio;
            }
            float startx = ImGui::GetCursorPosX();
            ImGui::SetCursorPosX(startx + offset);
#define SHOW_IMAGE(size)                                                    \
    ImGui::Image                                                            \
    (                                                                       \
        (void*)(uintptr_t)                                                  \
        (                                                                   \
            resource->type_ != Resource::Type::AnimatedTexture2D ?          \
            resource->id() :                                                \
            ((AnimatedTexture2DResource*)resource)->frameId()               \
        ),                                                                  \
        size,                                                               \
        {0,1},                                                              \
        {1,0}                                                               \
    );
            SHOW_IMAGE(previewSize)
            if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
            {
                SHOW_IMAGE(hoverSize)
                ImGui::EndTooltip();
            }
        };
        if 
        (
            resource->type_ == Resource::Type::Texture2D ||
            resource->type_ == Resource::Type::AnimatedTexture2D ||
            resource->type_ == Resource::Type::Framebuffer
        )
        {
            previewTexture2D(resource, 1.4*fontSize, 1.3*fontSize);
        }
        else if (resource->type_ == Resource::Type::Cubemap)
        {
            auto cubemap = (CubemapResource*)resource;
            for (int i=0; i<6; i++)
            {
                float offset = (i == 0 || i == 3) ? 0.75*fontSize : 0.0;
                previewTexture2D
                (
                    cubemap->unmanagedFaces_[i], 
                    0.5*fontSize, 
                    offset
                );
                if ((i+1)%3 != 0)
                    ImGui::SameLine();
            }
        }
        END_COLUMN
        START_COLUMN // Name column --------------------------------------------
        if (resource->type_ != Type::Framebuffer)
        {
            if (ImGui::InputText("##resourceName", resource->namePtr_))
                Helpers::enforceUniqueName
                (
                    *(resource->namePtr_),
                    resources,
                    resource
                );
        }
        else
            ImGui::Text(resource->name().c_str());
        END_COLUMN
        START_COLUMN // Resolution column --------------------------------------
        ImGui::Text("%d x %d", (int)x, (int)y);
        END_COLUMN
        START_COLUMN // Aspect ratio column ------------------------------------
        ImGui::Text("%.3f", aspectRatio);
        END_COLUMN
        END_ROW
    }; // End of renderResourceGUI lambda

    //--------------------------------------------------------------------------
    auto renderAddResourceButtonGUI = [&fontSize, &buttonWidth]
    (
        std::vector<Resource*>& resources,
        const int row
    )
    {
        int column = 0;
        START_ROW
        NEXT_COLUMN
        if (ImGui::Button(ICON_FA_PLUS, ImVec2(-1,0)))
            ImGui::OpenPopup("##addResourcePopup");
        if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
        {
            ImGui::Text("Add new resource");
            ImGui::EndTooltip();
        }
        if (ImGui::BeginPopup("##addResourcePopup"))
        {
            Resource* resource = nullptr;
            Resource::loadOrReplaceTextureOrAnimationButtonGUI
            (
                resource,
                ImVec2(buttonWidth, 0),
                false
            );
            Resource::loadOrReplaceTextureOrAnimationButtonGUI
            (
                resource,
                ImVec2(buttonWidth, 0),
                true
            );
            Resource::createOrEditAnimationButtonGUI
            (
                resource,
                resources,
                ImVec2(buttonWidth, 0)
            );
            Resource::createOrEditCubemapButtonGUI
            (
                resource,
                resources,
                ImVec2(buttonWidth, 0)
            );
            if (resource != nullptr)
            {
                if (resource->name().size() == 0)
                    resource->setName(Helpers::randomString(6));
                Helpers::enforceUniqueName
                (
                    *(resource->namePtr_),
                    resources
                );
                resources.emplace_back(resource);
            }
            ImGui::EndPopup();
        }
        END_ROW
    }; // End of renderAddResourceButtonGUI lambda
    
    //--------------------------------------------------------------------------
    static float tableHeight = 0;
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    if (ImGui::BeginTable("##resourceTable", 6, flags, ImVec2(0.0, tableHeight)))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("##controls", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Type", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Preview", flags, 4.0*fontSize);
        ImGui::TableSetupColumn("Name", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Resolution", flags, 8.0*fontSize);
        ImGui::TableSetupColumn
        (
            "Aspect ratio", 
            flags, 
            Resource::isGuiDetachedFromMenu ? 
            ImGui::GetContentRegionAvail().x : 8.0*fontSize
        );
        ImGui::TableHeadersRow();

        int deleteRow = -1;
        bool deleteResource = false;
        const int nRows = resources.size();
        for (int row=0; row<nRows; row++)
        {
            renderResourceGUI(deleteResource, resources, row);
            if (deleteResource)
            {
                deleteRow = row;
                deleteResource = false;
            }
        }
        renderAddResourceButtonGUI(resources, nRows);
        tableHeight = (ImGui::GetCursorPosY()-cursorPosY0);
        if (deleteRow != -1)
        {
            auto resource = resources[deleteRow];
            for (auto layer : layers)
                layer->removeResourceFromUniforms(resource);
            resources.erase(resources.begin()+deleteRow);
            delete resource;
        }
        ImGui::EndTable();
    }

    if (Resource::isGuiDetachedFromMenu)
        ImGui::End();
}

//----------------------------------------------------------------------------//

void Resource::renderResourcesMenuItemGUI
(
    std::vector<Resource*>& resources,
    const std::vector<Layer*>& layers
)
{
    if(ImGui::SmallButton(isGuiDetachedFromMenu ? "Z" : "O" ))
        isGuiDetachedFromMenu = !isGuiDetachedFromMenu;
    ImGui::SameLine();
    if (!isGuiDetachedFromMenu)
    {
        if (ImGui::BeginMenu("Resource manager"))
        {
            isGuiOpen = true;
            Resource::renderResourcesGUI(resources, layers);
            ImGui::EndMenu();
        }
        else
            isGuiOpen = false;
        return;
    }
    ImGui::MenuItem("Resource manager", NULL, &Resource::isGuiOpen);
}

bool Resource::insertFramebufferInResources
(
    std::string* name,
    vir::Framebuffer** framebuffer, 
    std::vector<Resource*>& resources
)
{
    for (int i=0; i<resources.size(); i++)
    {
        auto resource = resources[i];
        if (resource->type_ != Type::Framebuffer)
            continue;
        if (resource->id() == (*framebuffer)->id())
            return false;
    }
    resources.emplace_back(Resource::create(framebuffer))->setNamePtr(name);
    return true;
}

bool Resource::removeFramebufferFromResources
(
    vir::Framebuffer** framebuffer, 
    std::vector<Resource*>& resources
)
{
    for (int i=0; i<resources.size(); i++)
    {
        auto resource = resources[i];
        if (resource->type_ != Type::Framebuffer)
            continue;
        if (resource->id() == (*framebuffer)->id())
        {
            resources.erase(resources.begin()+i);
            return true;
        }
    }
    return false;
}

//----------------------------------------------------------------------------//

bool Resource::loadOrReplaceTextureOrAnimationButtonGUI
(
    Resource*& resource,
    const ImVec2 size,
    const bool animation,
    const bool disabled
)
{
    if (disabled)
        ImGui::BeginDisabled();
    if 
    (
        ImGui::Button
        (
            resource == nullptr ? 
            (animation ? "Load animation" : "Load texture") : 
            "Replace",
            size
        )
    )
    {
        animation ?
        Resource::fileDialog_.runOpenFileDialog
        (
            "Select a GIF", {"GIFs (.gif)", "*.gif"}, ".", (resource == nullptr)
        ) :
        Resource::fileDialog_.runOpenFileDialog
        (
            "Select an image",
            {
                "Image files (.png,.jpg,.jpeg,.bmp)", 
                "*.png *.jpg *.jpeg *.bmp"
            },
            ".",
            (resource == nullptr)
        );
        if (resource != nullptr)
            Resource::resourceToBeReplaced_ = &resource;
        return true;
    }
    if (disabled)
        ImGui::EndDisabled();
    return false;
}

//----------------------------------------------------------------------------//

bool Resource::createOrEditAnimationButtonGUI
(
    Resource*& resource,
    const std::vector<Resource*>& resources,
    const ImVec2 size
)
{
    bool valid = false;
    static int width(0);
    static int height(0);
    static std::vector<Texture2DResource*> frames(0);
    static std::vector<std::string> orderedFrameNames(0);
    if 
    (
        ImGui::Button
        (
            resource == nullptr ? 
            "Create animation" : 
            "Replace",
            size
        )
    )
    {
        ImGui::OpenPopup("##createOrEditAnimationPopup");
        width = 0;
        height = 0;
        if (resource != nullptr)
        {
            auto animation = (AnimatedTexture2DResource*)resource;
            frames = std::vector<Texture2DResource*>
            (
                animation->unmanagedFrames_
            );
            int n(animation->unmanagedFrames_.size());
            orderedFrameNames.resize(n);
            for (int i=0; i<n; i++)
                orderedFrameNames[i] = std::to_string(i+1) + " - " +
                    animation->unmanagedFrames_[i]->name();
        }
    }
    if (ImGui::BeginPopup("##createOrEditAnimationPopup"))
    {
        ImGui::SeparatorText("Animation frames");
        int nFrames = frames.size();
        if (width == 0 && height == 0 && nFrames == 1)
        {
            width = frames[0]->width();
            height = frames[0]->height();
        }
        static bool reordered(false);
        int iFrameToBeDeleted = -1;
        ImGui::BeginChild
        (
            "##framesChild", 
            ImVec2
            (
                ImGui::GetContentRegionAvail().x, 
                std::min
                (
                    (float)std::max(nFrames, 1), 
                    15.f
                )*
                ImGui::GetTextLineHeightWithSpacing()
            ), 
            false
        );
        for (int i=0; i<nFrames; i++)
        {
            auto* frame = frames[i];
            auto name = orderedFrameNames[i];
            ImGui::PushID(i); 
            if (ImGui::SmallButton(ICON_FA_TRASH))
                iFrameToBeDeleted=i;
            ImGui::PopID();
            ImGui::SameLine();
            ImGui::Selectable
            (
                name.c_str(), 
                false, 
                ImGuiSelectableFlags_DontClosePopups
            );
            if (ImGui::IsItemActive() && !ImGui::IsItemHovered())
            {
                int j = i + (ImGui::GetMouseDragDelta(0).y<0.f?-1:1);
                if (j >= 0 && j < nFrames)
                {
                    reordered = true;
                    orderedFrameNames[i] = orderedFrameNames[j];
                    orderedFrameNames[j] = name;
                    frames[i] = frames[j];
                    frames[j] = frame;
                    ImGui::ResetMouseDragDelta();
                }
            }
        }
        // As previously said, only re-order if the mouse button is no 
        // longer pressed
        if (!ImGui::IsMouseDown(ImGuiMouseButton_Left) && reordered)
        {
            for (int i=0; i<nFrames; i++)
                orderedFrameNames[i] = 
                    std::to_string(i+1)+" - "+
                    std::string(frames[i]->name());
            reordered = false;
        }
        ImGui::EndChild();
        if (iFrameToBeDeleted != -1)
        {
            frames.erase(frames.begin()+iFrameToBeDeleted);
            orderedFrameNames.erase
            (
                orderedFrameNames.begin()+iFrameToBeDeleted
            );
        }
        if
        (
            ImGui::BeginCombo
            (
                "##addAnimationFrameCombo",
                "Select frame to add"
            )
        )
        {
            for (auto* r : resources)
            {
                if 
                (
                    r->type_ != Resource::Type::Texture2D ||
                    (
                        width*height > 0 && 
                        (
                            r->width() != width ||
                            r->height() != height
                        )
                    )
                )
                    continue;
                if (ImGui::Selectable(r->name().c_str()))
                {
                    frames.emplace_back((Texture2DResource*)r);
                    orderedFrameNames.emplace_back
                    (
                        std::to_string(frames.size()) +
                        " - " + r->name()
                    );
                }
            }
            ImGui::EndCombo();
        }
        if (nFrames == 0)
            ImGui::BeginDisabled();
        if 
        (
            ImGui::Button
            (
                resource == nullptr ? "Create animation" : "Edit animation", 
                ImVec2(-1,0)
            )
        )
        {
            auto newResource = Resource::create(frames);
            if (newResource != nullptr)
            {
                if (resource != nullptr)
                    delete resource;
                resource = newResource;
                valid = true;
            }
            else
                valid = false;
            frames.clear();
            orderedFrameNames.clear();
        }
        if (nFrames == 0)
            ImGui::EndDisabled();
        ImGui::EndPopup();
    }
    return valid;
}

//----------------------------------------------------------------------------//

bool Resource::createOrEditCubemapButtonGUI
(
    Resource*& resource,
    const std::vector<Resource*>& resources,
    const ImVec2 size
)
{
    bool validSelection = false;
    static const Texture2DResource* selectedTextureResources[6]
    {
        nullptr, nullptr, nullptr, nullptr, nullptr, nullptr
    };
    static int width = 0;
    static int height = 0;
    if 
    (
        ImGui::Button
        (
            resource == nullptr ? "Create cubemap" : "Replace", 
            size
        )
    )
    {
        ImGui::OpenPopup("##createOrReplaceCubeMapPopup");
        width = 0;
        height = 0;
        if (resource == nullptr)
        {
            for (int i=0;i<6;i++)
                selectedTextureResources[i] = nullptr;
        }
        else
        {
            auto cubemap = (CubemapResource*)resource;
            for (int i=0;i<6;i++)
                selectedTextureResources[i] = 
                    cubemap->unmanagedFaces_[i];
        }
    }   
    if (ImGui::BeginPopup("##createOrReplaceCubeMapPopup"))
    {
        static std::string labels[6] = 
        {
            "X+  ", "X-  ", "Y+  ", "Y-  ", "Z+  ", "Z-  "
        };
        int textureResourcei(0);
        int nSelectedTextureResources(0);
        for (int i=0; i<6; i++)
        {
            if (selectedTextureResources[i] != nullptr)
            {
                nSelectedTextureResources++;
                textureResourcei = i;
            }
        }
        
        bool validFaces = true;
        for (int i=0; i<6; i++)
        {
            ImGui::Text(labels[i].c_str());
            ImGui::SameLine();
            std::string selectedTextureResourceName = 
                selectedTextureResources[i] != nullptr ?
                selectedTextureResources[i]->name() : "";
            ImGui::PushItemWidth(-1);
            std::string comboi = 
                "##cubeMapFaceResourceSelector"+std::to_string(i);
            if 
            (
                ImGui::BeginCombo
                (
                    comboi.c_str(), 
                    selectedTextureResourceName.c_str()
                )
            )
            {
                for(int j=0; j<resources.size()+1 ;j++)
                {
                    if (j==0)
                    {
                        if (ImGui::Selectable("None"))
                        {
                            selectedTextureResources[i] = nullptr;
                            if (nSelectedTextureResources == 1)
                            {
                                width=0;
                                height=0;
                            }
                        }
                        continue;
                    }
                    else if (resources[j-1]->type_ != Resource::Type::Texture2D)
                        continue;
                    auto r = (const Texture2DResource*)resources[j-1];
                    if 
                    (
                        !vir::CubeMapBuffer::validFace(r->native_)
                    )
                        continue;
                    if 
                    (
                        width != 0 && 
                        height != 0 && 
                        (
                            r->width() != width || 
                            r->height() != height
                        ) &&
                        !(
                            nSelectedTextureResources == 1 && 
                            i == textureResourcei
                        )
                    )
                        continue;
                    if (ImGui::Selectable(r->name().c_str()))
                    {
                        selectedTextureResources[i] = r;
                        width = r->width();
                        height = r->height();
                    }
                }
                ImGui::EndCombo();
            }
            ImGui::PopItemWidth();
            if (selectedTextureResources[i] == nullptr)
                validFaces = false;
        }
        float buttonSize(ImGui::GetFontSize()*15.0);
        if (!validFaces)
        {
            ImGui::BeginDisabled();
            ImGui::Button("Create cubemap", ImVec2(buttonSize, 0));
            ImGui::EndDisabled();
            if 
            (
                ImGui::IsItemHovered(ImGuiHoveredFlags_AllowWhenDisabled) && 
                ImGui::BeginTooltip()
            )
            {
                ImGui::Text(
R"(To generate a cube map, select a texture from the 
loaded Texture2D resources for each of the 6 faces 
of the cubemap. Said resources need to: 
    1) have a square aspect ratio; 
    2) have the same resolution; 
    3) have a resoluton which is a power of 2. 
The available textures are automatically filtered 
among those loaded in the resource manager.)");
                ImGui::EndTooltip();
            }
        }
        else if (ImGui::Button("Create cubemap", ImVec2(buttonSize, 0)))
        {
            auto newResource = Resource::create(selectedTextureResources);
            if (newResource != nullptr)
            {
                if (resource != nullptr)
                    delete resource;
                resource = newResource;
                validSelection = true;
            }
        }
        ImGui::EndPopup();
    }
    return validSelection;
}

//----------------------------------------------------------------------------//

bool Resource::exportTextureOrAnimationButtonGUI
(
    const Resource* resource,
    const ImVec2 size
)
{
    if 
    (
        resource == nullptr || 
        (
            resource->type_ != Resource::Type::Texture2D &&
            resource->type_ != Resource::Type::AnimatedTexture2D
        )
    )
        return false;
    if (ImGui::Button("Export", size))
    {
        std::string originalFileExtension = 
            resource->type_ == Resource::Type::Texture2D ?
            ((const Texture2DResource*)resource)->originalFileExtension_ :
            ((const AnimatedTexture2DResource*)resource)->originalFileExtension_;
        if (originalFileExtension.size() == 0)
        {
            if (resource->type_ == Resource::Type::Texture2D)
                originalFileExtension = ".png";
            else
                originalFileExtension = ".gif";
        }
        originalFileExtension = "*"+originalFileExtension;
        Resource::fileDialog_.runSaveFileDialog
        (
            "Re-export resource",
            {originalFileExtension, originalFileExtension},
            resource->name()
        );
        Resource::resourceToBeExported_ = resource;
        return true;
    }
    return false;
}

//----------------------------------------------------------------------------//

void Resource::renderResourceActionsButtonGUI
(
    Resource*& resource,
    bool& deleteResource,
    const std::vector<Resource*>& resources,
    const ImVec2 size
)
{
    if (resource->type_ != Resource::Type::Framebuffer)
    {
        if (ImGui::Button(ICON_FA_EDIT, ImVec2(-1,0)))
            ImGui::OpenPopup("##textureManagerSettings");
        if (ImGui::BeginPopup("##textureManagerSettings"))
        {
            std::vector<std::string*> inUseBy(0);
            if (resource->type_ == Resource::Type::Texture2D)
            {
                for (auto* r : resources)
                {
                    if (r->type_ == Resource::Type::Cubemap)
                    {
                        auto cubemap = (const CubemapResource*)r;
                        auto faces = cubemap->unmanagedFaces_;
                        for (int i=0; i<6; i++)
                        {
                            if (faces[i]->name() != resource->name())
                                continue;
                            inUseBy.emplace_back(cubemap->namePtr_);
                            break;
                        }
                    }
                    else if 
                    (
                        r->type_ == Resource::Type::AnimatedTexture2D
                    )
                    {
                        auto animation = 
                            (const AnimatedTexture2DResource*)r;
                        for (auto& frame : animation->unmanagedFrames_)
                        {
                            if (frame->name() != resource->name())
                                continue;
                            inUseBy.emplace_back(r->namePtr_);
                            break;
                        }
                    }
                }
            }
            //------------------------------------------------------------------

            if (ImGui::Button("Settings", size))
                ImGui::OpenPopup("##resourceSettings");
            if (ImGui::IsItemHovered() && inUseBy.size() > 0)
            {
                // Again, if inUseBy.size() > 0 I am assured this 
                // resource is a Texture2D, not a Cubemap nor an Animation
                ImGui::BeginTooltip();
                ImGui::Text(
R"(These settings only affect this texture and do not 
affect any cubemaps or animations using this texture)");
                ImGui::EndTooltip();
            }
            if (ImGui::BeginPopup("##resourceSettings"))
            {
                if (resource->type_ == Resource::Type::AnimatedTexture2D)
                {
                    //----------------------------------------------------------
                    auto animation = 
                        (AnimatedTexture2DResource*)resource;
                    auto nativeAnimation = animation->native_;
                    unsigned int frameIndex = nativeAnimation->frameIndex();
                    unsigned int nFrames = nativeAnimation->nFrames();
                    ImGui::Text("Animation frame     ");
                    ImGui::SameLine();
                    ImGui::BeginDisabled();
                    ImGui::Button
                    (   
                        std::to_string(frameIndex+1).c_str(),
                        ImVec2(size.x/3.25, 0)
                    );
                    ImGui::EndDisabled();
                    ImGui::SameLine();
                    ImGui::Text("/ %d", nFrames);
                    ImGui::Text("Animation timing    ");
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            animation->isAnimationBoundToGlobalTime_ ?
                            "Bound to iTime" :
                            "Manual control",
                            size
                        )
                    )
                    {
                        animation->isAnimationBoundToGlobalTime_ = 
                            !animation->isAnimationBoundToGlobalTime_;
                    }
                    if (animation->isAnimationBoundToGlobalTime_)
                        ImGui::BeginDisabled();
                    ImGui::Text("Animation controls  ");
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            ICON_FA_STEP_BACKWARD, 
                            ImVec2(size.x/3.25, 0)
                        )
                    )
                        nativeAnimation->previousFrame(); // Step backwards
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            animation->isAnimationPaused_ ? 
                            ICON_FA_PLAY : 
                            ICON_FA_PAUSE,
                            ImVec2(size.x/3.25, 0)
                        )
                    )
                        animation->isAnimationPaused_ = 
                            !animation->isAnimationPaused_;
                    ImGui::SameLine();
                    if 
                    (
                        ImGui::Button
                        (
                            ICON_FA_STEP_FORWARD, 
                            ImVec2(-1, 0)
                        )
                    )
                        nativeAnimation->nextFrame();
                    if (animation->isAnimationBoundToGlobalTime_)
                        ImGui::EndDisabled();
                    ImGui::Text("Animation FPS       ");
                    ImGui::SameLine();
                    float fps(nativeAnimation->fps());
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::DragFloat
                        (
                            "##animationFpsDragFloat",
                            &fps,
                            0.1f,
                            0.1f,
                            160.0f,
                            "%.1f"
                        )
                    )
                    {
                        fps = std::min(std::max(0.1f, fps), 1000.f);
                        nativeAnimation->setFps(fps);
                    }
                    ImGui::Text("Animation duration  ");
                    ImGui::SameLine();
                    float duration(nativeAnimation->duration());
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::DragFloat
                        (
                            "##animationDurationDragFloat",
                            &duration,
                            0.1f,
                            nFrames/1000.0f,
                            nFrames/.1f,
                            "%.3f"
                        )
                    )
                    {
                        fps = nFrames/duration;
                        nativeAnimation->setFps(fps);
                    }
                    ImGui::PopItemWidth();
                    ImGui::Separator();
                }
                std::string selectedWrapModeX = "";
                std::string selectedWrapModeY = "";
                std::string selectedMagFilterMode = "";
                std::string selectedMinFilterMode = "";
                selectedWrapModeX=vir::TextureBuffer::wrapModeToName.at
                (
                    resource->wrapMode(0)
                );
                selectedWrapModeY=vir::TextureBuffer::wrapModeToName.at
                (
                    resource->wrapMode(1)
                );
                selectedMagFilterMode = 
                    vir::TextureBuffer::filterModeToName.at
                    (
                        resource->magFilterMode()
                    );
                selectedMinFilterMode = 
                    vir::TextureBuffer::filterModeToName.at
                    (
                        resource->minFilterMode()
                    );
                if (resource->type_ != Resource::Type::Cubemap)
                {
                    ImGui::Text("Texture wrap mode H ");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::BeginCombo
                        (
                            "##bufferWrapModeXCombo",
                            selectedWrapModeX.c_str()
                        )
                    )
                    {
                        for (auto e:vir::TextureBuffer::wrapModeToName)
                            if (ImGui::Selectable(e.second.c_str()))
                                resource->setWrapMode(0, e.first);
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                    ImGui::Text("Texture wrap mode V ");
                    ImGui::SameLine();
                    ImGui::PushItemWidth(size.x);
                    if 
                    (
                        ImGui::BeginCombo
                        (
                            "##bufferWrapModeYCombo",
                            selectedWrapModeY.c_str()
                        )
                    )
                    {
                        for (auto e:vir::TextureBuffer::wrapModeToName)
                            if (ImGui::Selectable(e.second.c_str()))
                                resource->setWrapMode(1, e.first);
                        ImGui::EndCombo();
                    }
                    ImGui::PopItemWidth();
                }
                ImGui::Text("Texture mag. filter ");
                ImGui::SameLine();
                ImGui::PushItemWidth(size.x);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##bufferMagModeCombo",
                        selectedMagFilterMode.c_str()
                    )
                )
                {
                    for (auto e : vir::TextureBuffer::filterModeToName)
                    {
                        // Mipmap filters are for minimization only,
                        // no effect on magnification, hence they 
                        // are skipped here
                        if
                        (
                            e.first != 
                            vir::TextureBuffer::FilterMode::Nearest &&
                            e.first != 
                            vir::TextureBuffer::FilterMode::Linear
                        )
                            continue;
                        if (ImGui::Selectable(e.second.c_str()))
                            resource->setMagFilterMode(e.first);
                    }
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();

                ImGui::Text("Texture min. filter ");
                ImGui::SameLine();
                ImGui::PushItemWidth(size.x);
                if 
                (
                    ImGui::BeginCombo
                    (
                        "##bufferMinModeCombo",
                        selectedMinFilterMode.c_str()
                    )
                )
                {
                    for (auto e : vir::TextureBuffer::filterModeToName)
                        if (ImGui::Selectable(e.second.c_str()))
                            resource->setMinFilterMode(e.first);
                    ImGui::EndCombo();
                }
                ImGui::PopItemWidth();
                ImGui::EndPopup();
            }
            //------------------------------------------------------------------
            if (inUseBy.size() == 0)
            {
                switch (resource->type_)
                {
                    case Resource::Type::Texture2D:
                        loadOrReplaceTextureOrAnimationButtonGUI
                        (
                            resource,
                            size, 
                            false
                        );
                        break;
                    case Resource::Type::AnimatedTexture2D:
                        
                        ((AnimatedTexture2DResource*)resource)->
                        unmanagedFrames_.size() == 0 ? 
                        loadOrReplaceTextureOrAnimationButtonGUI
                        (
                            resource,
                            size, 
                            true
                        ) :
                        createOrEditAnimationButtonGUI
                        (
                            resource,
                            resources,
                            size
                        );
                        break;
                    case Resource::Type::Cubemap:
                        createOrEditCubemapButtonGUI
                        (
                            resource,
                            resources,
                            size
                        );
                        break;
                }
            }
            else    // No way inUseBy.size() != 0 if resource->type_ !=
                    // Resource::Type::Texture2D, so no cubemaps or 
                    // animations here
            {
                loadOrReplaceTextureOrAnimationButtonGUI
                (
                    resource,
                    size,
                    false,
                    true
                );
                if 
                (
                    ImGui::IsItemHovered
                    (
                        ImGuiHoveredFlags_AllowWhenDisabled
                    ) && ImGui::BeginTooltip()
                )
                {
                    std::string hoverText = 
"This texture is in use by the following resources:\n";
                    for (int i=0;i<inUseBy.size();i++)
                        hoverText += 
                        "  "+std::to_string(i+1)+") "+*inUseBy[i]+"\n";
                    hoverText += 
"To replace this texture, first delete the resources which use it";
                    ImGui::Text(hoverText.c_str());
                    ImGui::EndTooltip();
                }
            }
            //------------------------------------------------------------------
            if 
            (
                resource->type_ == Resource::Type::Texture2D ||
                resource->type_ == Resource::Type::AnimatedTexture2D
            )
                exportTextureOrAnimationButtonGUI(resource,size);
            //------------------------------------------------------------------
            if (inUseBy.size() == 0)
            {
                if (ImGui::Button("Delete", size))
                    deleteResource = true;
            }
            else
            {
                ImGui::BeginDisabled();
                ImGui::Button("Delete", size);
                ImGui::EndDisabled();
                if 
                (
                    ImGui::IsItemHovered
                    (
                        ImGuiHoveredFlags_AllowWhenDisabled
                    ) && ImGui::BeginTooltip()
                )
                {
                    std::string hoverText = 
"This texture is in use by the following resources:\n";
                    for (int i=0;i<inUseBy.size();i++)
                        hoverText += 
                        "  "+std::to_string(i+1)+") "+*inUseBy[i]+"\n";
                    hoverText += 
"To delete this texture, first delete the resources which use it";
                    ImGui::Text(hoverText.c_str());
                    ImGui::EndTooltip();
                }
            }
            ImGui::EndPopup();
        }
    }
}

}
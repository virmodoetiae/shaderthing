#include "shaderthing-p/include/resource.h"
#include "shaderthing-p/include/layer.h"
#include "shaderthing-p/include/helpers.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/icons/IconsFontAwesome5.h"
#include "thirdparty/stb/stb_image.h"

namespace ShaderThing
{

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

Resource* Resource::create(vir::Framebuffer* framebuffer)
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

bool FramebufferResource::set(vir::Framebuffer* framebuffer)
{
    if (framebuffer == nullptr)
        return false;
    native_ = &framebuffer;
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

void Resource::renderResourcesGUI(std::vector<Resource*>& resources)
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
        std::vector<Resource*>& resources,
        const int row
    )
    {
        Resource* resource = resources[row];
        int column = 0;
        START_ROW
        START_COLUMN // Actions column -----------------------------------------
        /*switch(resource->type_)
        {
            case Type::Texture2D :
                Resource::loadOrReplaceTextureOrAnimationButtonGUI
                (
                    resource,
                    ImVec2(-1, 0), 
                    false
                );
                break;
            case Type::AnimatedTexture2D :
                Resource::loadOrReplaceTextureOrAnimationButtonGUI
                (
                    resource,
                    ImVec2(-1, 0), 
                    true
                );
                break;
        }
        resources[row] = resource;*/
        renderResourceActionsGUI(resource);
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
        ImGui::InputText("##resourceName", resource->namePtr_);
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
    auto renderAddResourceButtonGUI = 
    [
        &fontSize, 
        &buttonWidth, 
        &cursorPosY0
    ]
    (
        std::vector<Resource*>& resources,
        const int row,
        float& tableHeight
    )
    {
        int column = 0;
        START_ROW
        START_COLUMN
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
            if (resource != nullptr)
                resources.emplace_back(resource);
            ImGui::EndPopup();
        }
        tableHeight = (ImGui::GetCursorPosY()-cursorPosY0);
        END_COLUMN
        END_ROW
    }; // End of renderAddResourceButtonGUI lambda
    
    //--------------------------------------------------------------------------
    static float tableHeight = 0;
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    Resource* toBeDeleted = nullptr;
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

        const int nRows = resources.size();
        for (int row=0; row<nRows; row++)
            renderResourceGUI(resources, row);
        renderAddResourceButtonGUI(resources, nRows, tableHeight);

        ImGui::EndTable();
    }

    if (Resource::isGuiDetachedFromMenu)
        ImGui::End();
}

//----------------------------------------------------------------------------//

void Resource::renderResourcesMenuItemGUI(std::vector<Resource*>& resources)
{
    if(ImGui::SmallButton(isGuiDetachedFromMenu ? "Z" : "O" ))
        isGuiDetachedFromMenu = !isGuiDetachedFromMenu;
    ImGui::SameLine();
    if (!isGuiDetachedFromMenu)
    {
        if (ImGui::BeginMenu("Resource manager"))
        {
            isGuiOpen = true;
            Resource::renderResourcesGUI(resources);
            ImGui::EndMenu();
        }
        else
            isGuiOpen = false;
        return;
    }
    ImGui::MenuItem("Resource manager", NULL, &Resource::isGuiOpen);
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
    static std::string lastOpenedPath(".");
    bool validSelection = false;
    std::string dialogKey;
    if (resource == nullptr)
        dialogKey = animation ? "##loadAnimationDialog" : "##loadTextureDialog";
    else
    {
        dialogKey = 
            animation ? 
            "##replaceAnimationDialog"+std::to_string(resource->id()) : 
            "##replaceTextureDialog"+std::to_string(resource->id());
    }
    if (disabled)
        ImGui::BeginDisabled();
    auto fileDialog(ImGuiFileDialog::Instance());
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
        ImGui::SetNextWindowSize(ImVec2(900, 350), ImGuiCond_FirstUseEver);
        fileDialog->OpenDialog
        (
            dialogKey.c_str(), 
            ICON_FA_IMAGE " Select an image", 
            animation ? 
            ".gif" : 
            "Image files (*.png *.jpg *.jpeg *.bmp){.png,.jpg,.jpeg,.bmp}",
            lastOpenedPath
        );
    }
    if (disabled)
        ImGui::EndDisabled();
    if (fileDialog->Display(dialogKey))
    {
        if (fileDialog->IsOk())
        {
            lastOpenedPath = fileDialog->GetCurrentPath()+"/";
            auto newResource = Resource::create(fileDialog->GetFilePathName());
            if (newResource != nullptr)
            {
                if (resource != nullptr)
                    delete resource;
                resource = newResource;
                resource->setName(fileDialog->GetCurrentFileName());
                validSelection = true;
            }
        }
        fileDialog->Close();
    }
    return validSelection;
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
                    *(animation->unmanagedFrames_[i]->namePtr_);
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
                    std::string(*(frames[i]->namePtr_));
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
                if (ImGui::Selectable(r->namePtr_->c_str()))
                {
                    frames.emplace_back((Texture2DResource*)r);
                    orderedFrameNames.emplace_back
                    (
                        std::to_string(frames.size()) +
                        " - " + *(r->namePtr_)
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
                resource->setName("newAnimation");
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
    const ImVec2 size
)
{
    return false;
}

//----------------------------------------------------------------------------//

bool Resource::exportTextureOrAnimationButtonGUI
(
    const Resource* resource,
    const ImVec2 size
)
{
    return false;
}

//----------------------------------------------------------------------------//

void Resource::renderResourceActionsGUI
(
    Resource*& resource
)
{
}

}
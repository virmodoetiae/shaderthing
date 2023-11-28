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
#include "misc/misc.h"

#include "thirdparty/imgui/imgui.h"
#include "thirdparty/imgui/misc/cpp/imgui_stdlib.h"
#include "thirdparty/imguifiledialog/ImGuiFileDialog.h"
#include "thirdparty/stb/stb_image.h"

namespace ShaderThing
{

//----------------------------------------------------------------------------//
// Public functions ----------------------------------------------------------//

void ResourceManager::renderGui()
{
    if (!isGuiOpen_)
        return;

    if (!isGuiInMenu_)
    {
        ImGui::SetNextWindowSize(ImVec2(900,350), ImGuiCond_FirstUseEver);
        static ImGuiWindowFlags windowFlags
        (
            ImGuiWindowFlags_NoCollapse
        );
        ImGui::Begin("Resource manager", &isGuiOpen_, windowFlags);
        static bool setIcon(false);
        if (!setIcon)
        {
            setIcon = vir::ImGuiRenderer::setWindowIcon
            (
                "Resource manager", 
                IconData::sTIconData, 
                IconData::sTIconSize,
                false
            );
        }
    }
    
    float fontSize(ImGui::GetFontSize());
    
    static float tableHeight = 0;
    ImGuiTableFlags flags = 
        ImGuiTableFlags_BordersV | 
        ImGuiTableFlags_BordersOuterH |
        ImGuiTableFlags_SizingFixedFit;
    Resource* toBeDeleted = nullptr;
    float y0 = ImGui::GetCursorPosY();
    if (ImGui::BeginTable("##resourceTable", 6, flags, ImVec2(0.0, tableHeight)))
    {
        // Declare columns
        static ImGuiTableColumnFlags flags = 0;
        ImGui::TableSetupColumn("Actions ", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Type", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Preview", flags, 4.0*fontSize);
        ImGui::TableSetupColumn("Name", flags, 8.0*fontSize);
        ImGui::TableSetupColumn("Resolution", flags, 8.0*fontSize);
        ImGui::TableSetupColumn
        (
            "Aspect ratio", 
            flags, 
            isGuiInMenu_ ? 
            8.0*fontSize : ImGui::GetContentRegionAvail().x
        );
        ImGui::TableHeadersRow();
        
        // Actual rendering ----------------------------------------------------
        int nRows = (int)resources_.size();
        for (int row = 0; row < nRows+1; row++)
        {
            int col = 0;
            ImGui::PushID(row);
            ImGui::TableNextRow(0, 1.6*fontSize);
            
            if (row == nRows)
            {
                ImGui::TableSetColumnIndex(col++);
                ImGui::PushItemWidth(-1);
                if (ImGui::Button("Add", ImVec2(-1,0)))
                    ImGui::OpenPopup("##addResourcePopup");
                if (ImGui::BeginPopup("##addResourcePopup"))
                {
                    addOrReplaceTextureGuiButton
                    (
                        -1,
                        ImVec2(8*fontSize, 0)
                    );
                    addOrReplaceCubemapGuiButton
                    (
                        -1,
                        ImVec2(8*fontSize, 0)
                    );
                    ImGui::EndPopup();
                }
                
                ImGui::PopItemWidth();
                ImGui::PopID();
                tableHeight = (ImGui::GetCursorPosY()-y0);
                break;
            }
            
            auto resource = resources_[row];

            // Column 0 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            float buttonWidth(10*fontSize);
            if (resource->type() != Resource::Type::FramebufferColorAttachment)
            {
                bool replacedResource(false);
                if (ImGui::Button("Actions", ImVec2(-1,0)))
                    ImGui::OpenPopup("##textureManagerActions");
                if (ImGui::BeginPopup("##textureManagerActions"))
                {
                    std::vector<std::string*> inUseBy(0);
                    if (resource->type() == Resource::Type::Texture2D)
                    {
                        for (auto* r : resources_)
                        {
                            if (r->type() != Resource::Type::Cubemap) continue;
                            auto& faces = r->referencedResourcesCRef();
                            if (faces.size() != 6) continue;
                            for (auto& f : faces)
                                if (f->nameCPtr() == resource->namePtr())
                                    inUseBy.emplace_back(r->namePtr());
                        }
                    }
                    if (inUseBy.size() == 0)
                    {
                        if (ImGui::Button("Delete", ImVec2(buttonWidth,0)))
                            toBeDeleted = resource;
                    }
                    else
                    {
                        ImGui::BeginDisabled();
                        ImGui::Button("Delete", ImVec2(buttonWidth,0));
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
"This texture is in use by the following cube maps:\n";
                            for (int i=0;i<inUseBy.size();i++)
                                hoverText += 
                                "  "+std::to_string(i+1)+") "+*inUseBy[i]+"\n";
                            hoverText += 
"To delete this texture, first delete the cube maps which use it";
                            ImGui::Text(hoverText.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    if (inUseBy.size() == 0)
                    {
                        if (resource->type() == Resource::Type::Texture2D)
                            replacedResource = addOrReplaceTextureGuiButton
                            (
                                row,
                                ImVec2(buttonWidth, 0)
                            );
                        else if (resource->type() == Resource::Type::Cubemap)
                            replacedResource = addOrReplaceCubemapGuiButton
                            (
                                row,
                                ImVec2(buttonWidth, 0)
                            );
                    }
                    else    // No way inUseBy.size() != 0 if resource->type() !=
                            // Resource::Type::Texture2D, so no cubemap stuff 
                            // here
                    {
                        ImGui::BeginDisabled();
                        replacedResource = addOrReplaceTextureGuiButton
                        (
                            row,
                            ImVec2(buttonWidth, 0),
                            true
                        );
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
"This texture is in use by the following cube maps:\n";
                            for (int i=0;i<inUseBy.size();i++)
                                hoverText += 
                                "  "+std::to_string(i+1)+") "+*inUseBy[i]+"\n";
                            hoverText += 
"To replace this texture, first delete the cube maps which use it";
                            ImGui::Text(hoverText.c_str());
                            ImGui::EndTooltip();
                        }
                    }
                    if (resource->type() == Resource::Type::Texture2D)
                    {
                        reExportTextureGuiButton
                        (
                            resource,
                            ImVec2(buttonWidth,0)
                        );
                        if 
                        (
                            ImGui::Button("Data to clipboard", 
                            ImVec2(buttonWidth,0))
                        )
                        {
                            std::stringstream data;
                            data << "const unsigned char imageData[" 
                                << resource->rawDataSize() 
                                << "] = {";
                            for (int i=0; i< resource->rawDataSize(); i++)
                                data << (int)resource->rawData()[i] << ",";
                            data << "};";
                            ImGui::SetClipboardText(data.str().c_str());
                            data.str(std::string());
                        }
                    }
                    if (ImGui::Button("Options", ImVec2(buttonWidth,0)))
                        ImGui::OpenPopup("##resourceOptions");
                    if (ImGui::IsItemHovered() && inUseBy.size() > 0)
                    {
                        // Again, if inUseBy.size() > 0 I am assured this 
                        // resource is a Texture2D, not a Cubemap
                        ImGui::BeginTooltip();
                        ImGui::Text(
R"(These options only affect this texture and do
not affect any cubemaps using this texture)");
                        ImGui::EndTooltip();
                    }
                    if (ImGui::BeginPopup("##resourceOptions"))
                    {
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
                        ImGui::Text("Texture wrap mode H ");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(buttonWidth);
                        if 
                        (
                            ImGui::BeginCombo
                            (
                                "##bufferWrapModeXCombo",
                                selectedWrapModeX.c_str()
                            )
                        )
                        {
                            for (auto e : vir::TextureBuffer::wrapModeToName)
                                if (ImGui::Selectable(e.second.c_str()))
                                    resource->setWrapMode(0, e.first);
                            ImGui::EndCombo();
                        }
                        ImGui::PopItemWidth();
                        ImGui::Text("Texture wrap mode V ");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(buttonWidth);
                        if 
                        (
                            ImGui::BeginCombo
                            (
                                "##bufferWrapModeYCombo",
                                selectedWrapModeY.c_str()
                            )
                        )
                        {
                            for (auto e : vir::TextureBuffer::wrapModeToName)
                                if (ImGui::Selectable(e.second.c_str()))
                                    resource->setWrapMode(1, e.first);
                            ImGui::EndCombo();
                        }
                        ImGui::PopItemWidth();

                        ImGui::Text("Texture mag. filter ");
                        ImGui::SameLine();
                        ImGui::PushItemWidth(buttonWidth);
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
                        ImGui::PushItemWidth(buttonWidth);
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
                    if (replacedResource)
                        ImGui::CloseCurrentPopup();
                    ImGui::EndPopup();
                }
            }
            ImGui::PopItemWidth();

            // Column 1 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            ImGui::Text(Resource::typeToName.at(resource->type()).c_str());
            ImGui::PopItemWidth();

            // Column 2 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            float x = resource->width();
            float y = resource->height();
            float aspectRatio = x/y;
            auto previewTexture2D = [&]
            (
                Resource* r, 
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
                ImVec2 uv0{0,1};
                ImVec2 uv1{1,0};
                ImGui::Image
                (
                    (void*)(uintptr_t)(r->id()), 
                    previewSize, 
                    uv0, 
                    uv1
                );
                if (ImGui::IsItemHovered() && ImGui::BeginTooltip())
                {
                    ImGui::Image
                    (
                        (void*)(uintptr_t)(r->id()), 
                        hoverSize, 
                        uv0, 
                        uv1
                    );
                    ImGui::EndTooltip();
                }
            };
            if 
            (
                resource->type() == Resource::Type::Texture2D ||
                resource->type() == Resource::Type::AnimatedTexture2D ||
                resource->type() == Resource::Type::FramebufferColorAttachment
            )
            {
                previewTexture2D(resource, 1.4*fontSize, 1.3*fontSize);
            }
            else if (resource->type() == Resource::Type::Cubemap)
            {
                int i = 0;
                for (auto face : resource->referencedResourcesCRef())
                {
                    float offset = (i == 0 || i == 3) ? 0.75*fontSize : 0.0;
                    previewTexture2D(face, 0.5*fontSize, offset);
                    if ((i+1)%3 != 0)
                        ImGui::SameLine();
                    i++;
                }
            }
            ImGui::PopItemWidth();

            // Column 3 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            if (resource->type() == Resource::Type::FramebufferColorAttachment)
                ImGui::Text(resource->name().c_str());
            else
            {
                ImGui::InputText("##name", resource->namePtr());
                Misc::enforceUniqueItemName
                (
                    *resource->namePtr(), 
                    resource, 
                    app_.resourcesRef()
                );
            }
            ImGui::PopItemWidth();

            // Column 4 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            std::string resolution = 
                std::to_string(resource->width()) + " x " + 
                std::to_string(resource->height());
            ImGui::Text(resolution.c_str());
            ImGui::PopItemWidth();

            // Column 4 --------------------------------------------------------
            ImGui::TableSetColumnIndex(col++);
            ImGui::PushItemWidth(-1);
            ImGui::Text("%.3f", aspectRatio);
            ImGui::PopItemWidth();
            ImGui::PopID();
        }
        ImGui::EndTable();
    }

    // Lagged deletion
    if (toBeDeleted != nullptr)
    {
        if (toBeDeleted->type() != Resource::Type::FramebufferColorAttachment)
            app_.layerManagerRef().removeResourceFromUniforms(toBeDeleted);
        resources_.erase
        (
            std::remove(resources_.begin(), resources_.end(), toBeDeleted),
            resources_.end()
        );
        delete toBeDeleted;
    }
    
    if (!isGuiInMenu_)
        ImGui::End();
}

//----------------------------------------------------------------------------//
// Private functions ---------------------------------------------------------//

bool ResourceManager::addOrReplaceTextureGuiButton
(
    int rowIndex,
    ImVec2 size,
    bool disabled
)
{
    static int sIndex;
    bool validSelection(false);
    static std::string lastOpenedPath(".");
    std::string buttonName = (rowIndex==-1) ? 
        "Add texture" : "Replace texture";
    static std::string dialogKey("##addOrReplaceTextureDialog");
    ImGui::BeginDisabled(disabled);
    if (ImGui::Button(buttonName.c_str(), size))
    {
        ImGui::SetNextWindowSize(ImVec2(900,350), ImGuiCond_FirstUseEver);
        sIndex = rowIndex;
        ImGuiFileDialog::Instance()->OpenDialog
        (
            dialogKey.c_str(), 
            "Choose image", 
            ".png,.jpg,.jpeg,.bmp,.gif", 
            lastOpenedPath
        );
    }
    ImGui::EndDisabled();
    if (ImGuiFileDialog::Instance()->Display(dialogKey)) 
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filename = 
                ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string filepath = 
                ImGuiFileDialog::Instance()->GetFilePathName();
            lastOpenedPath = 
                ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
            try
            {
                Resource* r = nullptr;
                if (sIndex == -1)
                {
                    r = new Resource();
                    resources_.push_back(r);
                }
                else
                {
                    r = resources_[sIndex];
                    if (r != nullptr)
                        r->reset();
                    else
                    {
                        r = new Resource();
                        resources_.push_back(r);
                    }
                }
                r->setNamePtr(new std::string(filename));
                r->set(filepath);
                validSelection = true;
            }
            catch (const std::exception& e)
            {
                std::cout << "Failed to load texture from " << filepath 
                << std::endl;
            }
        }
        ImGuiFileDialog::Instance()->Close();
    }
    return validSelection;
}

//----------------------------------------------------------------------------//

bool ResourceManager::addOrReplaceCubemapGuiButton
(
    int rowIndex,
    ImVec2 size
)
{
    bool validSelection = false;
    static int sIndex;
    static Resource* selectedTextureResources[6]
    {
        nullptr,nullptr,nullptr,nullptr,nullptr,nullptr
    };
    static int width = 0;
    static int height = 0;
    if 
    (
        ImGui::Button
        (
            rowIndex == -1 ? "Add cubemap" : "Replace cubemap", 
            size
        )
    )
    {
        ImGui::OpenPopup("##addOrReplaceCubeMapPopup");
        sIndex = rowIndex;
        width = 0;
        height = 0;
        for (int i=0;i<6;i++)
            selectedTextureResources[i] = nullptr;
    }
    if (ImGui::BeginPopup("##addOrReplaceCubeMapPopup"))
    {
        static std::string labels[6] = 
        {
            "X+  ", "X-  ", "Y+  ", "Y-  ", "Z+  ", "Z-  "
        };
        int textureResourcei(0);
        int nSelectedTextureResources(0);
        for (int i=0;i<6;i++)
        {
            if (selectedTextureResources[i] != nullptr)
            {
                nSelectedTextureResources++;
                textureResourcei = i;
            }
        }
        
        bool validFaces = true;
        for (int i=0;i<6;i++)
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
                for(int j=0;j<resources_.size()+1;j++)
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
                    else if (resources_[j-1]->type()!=Resource::Type::Texture2D)
                        continue;
                    auto r = resources_[j-1];
                    if (!Resource::validCubemapFace(r))
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
            ImGui::Button("Generate cubemap", ImVec2(buttonSize, 0));
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
        else if (ImGui::Button("Generate cubemap", ImVec2(buttonSize,0)))
        {
            Resource* resource;
            if (sIndex != -1)
                resource = resources_[sIndex];
            else
            {
                resource = new Resource();
                resources_.emplace_back(resource);
            }
            resource->set(selectedTextureResources);
            Misc::enforceUniqueItemName
            (
                *resource->namePtr(), 
                resource, 
                app_.resourcesRef()
            );
            validSelection = true;
        }
        ImGui::EndPopup();
    }
    return validSelection;
}

//----------------------------------------------------------------------------//

bool ResourceManager::reExportTextureGuiButton
(
    Resource* resource,
    ImVec2 size
)
{
    bool validExport = false;
    if (resource == nullptr)
        return validExport;
    if 
    (
        resource->type() != Resource::Type::Texture2D &&
        resource->type() != Resource::Type::AnimatedTexture2D
    )
        return validExport;
    static std::string lastOpenedPath(".");
    if (ImGui::Button("Re-export", size))
    {
        ImGui::SetNextWindowSize(ImVec2(800,400), ImGuiCond_FirstUseEver);
        ImGuiFileDialog::Instance()->OpenDialog
        (
            "##reExportTextureDialog", 
            "Provide re-export filepath", 
            resource->originalFileExtension().c_str(), 
            lastOpenedPath
        );
    }
    if (ImGuiFileDialog::Instance()->Display("##reExportTextureDialog")) 
    {
        if (ImGuiFileDialog::Instance()->IsOk())
        {
            std::string filename = 
                ImGuiFileDialog::Instance()->GetCurrentFileName();
            std::string filepath = 
                ImGuiFileDialog::Instance()->GetFilePathName();
            lastOpenedPath = 
                ImGuiFileDialog::Instance()->GetCurrentPath()+"/";
            
            std::ofstream file;
            file.open(filepath, std::ios_base::out|std::ios_base::binary);
            if(!file.is_open())
                return false;
            file.write
            (
                (const char*)resource->rawData(), resource->rawDataSize()
            );
            file.close();
            validExport = true;
        }
        ImGuiFileDialog::Instance()->Close();
    }
    return validExport;
}

}
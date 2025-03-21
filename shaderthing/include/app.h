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

#pragma once

#include <vector>
#include "vir/include/vir.h"
#include "shaderthing/include/macros.h"
#include "shaderthing/include/filedialog.h"

class ImFontConfig;

namespace ShaderThing
{

class SharedUniforms;
class Layer;
class Resource;
class Exporter;
class FileDialog;
class ObjectIO;

class App
{
private:

    struct Project
    {
        enum class Action
        {
            None,
            New,
            Load,
            LoadExample,
            Save,
            SaveAs
        };
        Action             action            = Action::None;
        std::string        filepath          = "";
        std::string        filename          = "untitled.stf";
        const std::string* exampleToBeLoaded = nullptr;
        bool               forceSaveAs       = true;
        bool               isAutoSaveEnabled = true;
        float              timeSinceLastSave = 0.f;
        float              autoSaveInterval  = 60.f;
        void               renderAutoSaveMenuItemGui();
    };
    mutable Project        project_         = {};
    std::string            imGuiTitle_      = "Control panel###CP";
    SharedUniforms*        sharedUniforms_  = nullptr;
    std::vector<Layer*>    layers_          = {};
    std::vector<Resource*> resources_       = {};
    Exporter*              exporter_        = nullptr;
    FileDialog             fileDialog_;

    struct WindowSettings
    {
        bool               isVSyncEnabled   = true;
        float              lowerFpsLimit    = 5.0;
    };
    WindowSettings         windowSettings_  = {};

    struct Font
    {
        ImFont*            font             = nullptr;
        float*             fontScale        = nullptr;
        ImFontConfig       fontConfig       = {}; 
        bool               isJapaneseLoaded = false;
        bool               loadJapanese     = false;
        bool      isSimplifiedChineseLoaded = false;
        bool      loadSimplifiedChinese     = false;
        void      initialize();
        void      checkLoadJapaneseAndOrSimplifiedChinese();
        void      renderMenuItemGui();
    };
    Font                   font_            = {};
    
    void saveProject(const std::string& filepath, bool isAutosave) const;
    void loadProject(const std::string& filepathOrData, bool fromMemory=false);
    void newProject();
    void processProjectActions();

    void update();
    void renderGui();
    void renderMenuBarGui();
    
public:

    App();
    ~App();
};

}
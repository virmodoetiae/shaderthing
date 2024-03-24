/*
 _____________________
|                     |  This file is part of ShaderThing - A GUI-based live
|   ___  _________    |  shader editor by Stefan Radman (a.k.a., virmodoetiae).
|  /\  \/\__    __\   |  For more information, visit:
|  \ \  \/__/\  \_/   |
|   \ \__   \ \  \    |  https://github.com/virmodoetiae/shaderthing
|    \/__/\  \ \  \   |
|        \ \__\ \__\  |  SPDX-FileCopyrightText:    2024 Stefan Radman
|  Ↄ|C    \/__/\/__/  |                             sradman@protonmail.com
|  Ↄ|C                |  SPDX-License-Identifier:   Zlib
|_____________________|

*/

#pragma once

#include <string>
#include <future>
#include <thread>
#include <vector>

namespace pfd
{
enum class opt : uint8_t;
}

namespace ShaderThing
{

class FileDialog
{
private:
    enum class Type
    {
        OpenFile,
        SaveFile
    };
    bool                                  isOpen_       = false;
    bool                                  isBlocking_   = false;
    void*                                 nativeDialog_ = nullptr;
    std::vector<std::string>              selection_;
    std::future<std::vector<std::string>> futureSelection_;
    std::thread                           thread_;
    auto dialogTask
    (
              Type                      type,
        const std::string&              title,
        const std::vector<std::string>& filters, 
        const std::string&              defaultPath,
        const pfd::opt                  opt
    );
public:
    static FileDialog instance;
    ~FileDialog();
    void runOpenFileDialog
    (
        const std::string&              title = "Open file",
        const std::vector<std::string>& filters = {"All files", "*"}, 
        const std::string&              defaultPath = ".",
        const bool                      multipleSelection = false,
        const bool                      blocking = false
    );
    void runSaveFileDialog
    (
        const std::string&              title = "Save file",
        const std::vector<std::string>& filters = {"All files", "*"}, 
        const std::string&              defaultPath = ".",
        const bool                      blocking = false
    );
    bool isOpen() const {return isOpen_;}
    bool validSelection();
    void clearSelection() {selection_.clear();}
    const std::vector<std::string>& selection() const {return selection_;}
};
    
}
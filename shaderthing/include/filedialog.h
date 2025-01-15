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

// Weird crashes can happen (rarely) when optimization > O0 are tunred on,
// hence I am forcing some of the member functions to be compiled without
// optimizations. This is all on me, but I could not figure out what is
// happening, so this is just a poor man's patch to fix the issue. This class,
// which is basically just a multi-threaded wrapper around 
// portable-file-dialogs, will have to be revisted at some point

//
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
        const bool                      blocking = true
    );
    void runSaveFileDialog
    (
        const std::string&              title = "Save file",
        const std::vector<std::string>& filters = {"All files", "*"}, 
        const std::string&              defaultPath = ".",
        const bool                      blocking = true
    );
    bool validSelection();
    
    void clearSelection() {selection_.clear();}
    bool isOpen() const {return isOpen_;}
    const std::vector<std::string>& selection() const {return selection_;}
};
    
}
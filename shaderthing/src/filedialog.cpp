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

#include <algorithm>
#include <chrono>

#include "shaderthing/include/filedialog.h"

#include "shaderthing/include/helpers.h"

#include "thirdparty/portable-file-dialogs/pfd.h"

namespace ShaderThing
{

// Static instance for ease of usage
FileDialog FileDialog::instance;

FileDialog::~FileDialog()
{
    if (thread_.joinable())
        thread_.join();
}

auto FileDialog::dialogTask
(
    Type type,
    const std::string& title,
    const std::vector<std::string>& filters, 
    const std::string& defaultPath,
    const pfd::opt opt
)
{
    void** nativeDialogPtr = &nativeDialog_;
    return std::packaged_task<std::vector<std::string>()>
    (
        [nativeDialogPtr, type, title, filters, defaultPath, opt]
        {
#define START_DIALOG_ASYNC(type)                                            \
    (*nativeDialogPtr) = (void*)new type(title, defaultPath, filters, opt); \
    auto dialog = (type*)(*nativeDialogPtr);
            std::vector<std::string> selection;
            if (type == Type::OpenFile)
            {
                START_DIALOG_ASYNC(pfd::open_file)
                while (true)
                {
                    if (!dialog->ready(1000)) // Check every 1000 ms
                        continue;
                    selection = dialog->result();
                    break;
                }
                delete dialog;
            }
            else // if type == Type::SaveFile
            {
                START_DIALOG_ASYNC(pfd::save_file)
                while (true)
                {
                    if (!dialog->ready(1000)) // Check every 1000 ms
                        continue;
                    auto result = dialog->result();
                    if (result.size() > 0)
                    {
                        // Auto try add file extension if not present in 
                        // selection
                        auto fileExtension = Helpers::fileExtension(result);
                        if (fileExtension.size() < 2 && filters.size() > 1)
                        {
                            fileExtension = Helpers::fileExtension
                            (
                                filters.at(1)
                            );
                            if (fileExtension.size() > 1)
                                result += fileExtension;
                        }
                        selection.emplace_back(result);
                    }
                    break;
                }
                delete dialog;
            }
            (*nativeDialogPtr) = nullptr;
            return selection;
        }
    );
}

void FileDialog::runOpenFileDialog
(
    const std::string& title,
    const std::vector<std::string>& filters, 
    const std::string& defaultPath,
    const bool multipleSelection,
    const bool blocking
)
{
    if (isOpen_)
        return;
    selection_.clear();
    isBlocking_ = blocking;
    auto opt = multipleSelection ? pfd::opt::multiselect : pfd::opt::none;
    if (!isBlocking_)
    {
        isOpen_ = true;
        auto task = dialogTask(Type::OpenFile, title, filters, defaultPath, opt);
        futureSelection_ = task.get_future();
        thread_ = std::thread(std::move(task));
    }
    else
    {
        selection_ = pfd::open_file(title, defaultPath, filters, opt).result();
        isOpen_ = false;
    }
}

void FileDialog::runSaveFileDialog
(
    const std::string& title,
    const std::vector<std::string>& filters, 
    const std::string& defaultPath,
    const bool blocking
)
{
    if (isOpen_)
        return;
    selection_.clear();
    isBlocking_ = blocking;
    auto opt = pfd::opt::none;
    if (!isBlocking_)
    {
        isOpen_ = true;
        auto task = dialogTask(Type::SaveFile, title, filters, defaultPath, opt);
        futureSelection_ = task.get_future();
        thread_ = std::thread(std::move(task));
    }
    else
    {
        auto result = pfd::save_file(title, defaultPath, filters, opt).result();
        if (result.size() > 0)
        {
            // Auto try add file extension if not present in selection
            auto fileExtension = Helpers::fileExtension(result);
            if (fileExtension.size() < 2 && filters.size() > 1)
            {
                fileExtension = Helpers::fileExtension(filters.at(1));
                if (fileExtension.size() > 1)
                    result += fileExtension;
            }
            selection_.emplace_back(result);
        }
        isOpen_ = false;
    }
}

bool FileDialog::validSelection()
{
    if (!isBlocking_)
    {
        if (!isOpen_)
            return false;
        auto status = futureSelection_.wait_for(std::chrono::nanoseconds(0));
        if (status != std::future_status::ready)
            return false;
        isOpen_ = false;
        if (thread_.joinable())
            thread_.join();
        if (futureSelection_.valid()) 
        {
            try 
            {
                selection_ = futureSelection_.get();
            } 
            catch(const std::exception& e) 
            {
                std::cerr 
                    << "ShaderThing::FileDialog::validSelection(): "
                    << e.what() << std::endl;
                selection_.clear();
            }
        }
    }
    return selection_.size() > 0;
}

}

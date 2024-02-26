#include "shaderthing-p/include/filedialog.h"
#include "thirdparty/portable-file-dialogs/pfd.h"
#include "algorithm"
#include <chrono>

namespace ShaderThing
{

// Static instance for ease of usage
FileDialog FileDialog::instance;

FileDialog::~FileDialog()
{
    if (isOpen_)
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
                    selection.emplace_back(dialog->result());
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
    const bool multipleSelection
)
{
    if (isOpen_)
        return;
    isOpen_ = true;
    auto task = dialogTask
    (
        Type::OpenFile,
        title,
        filters, 
        defaultPath,
        multipleSelection ? pfd::opt::multiselect : pfd::opt::none
    );
    futureSelection_ = task.get_future();
    thread_ = std::thread(std::move(task));
}

void FileDialog::runSaveFileDialog
(
    const std::string& title,
    const std::vector<std::string>& filters, 
    const std::string& defaultPath
)
{
    if (isOpen_)
        return;
    isOpen_ = true;
    auto task = dialogTask
    (
        Type::SaveFile,
        title,
        filters, 
        defaultPath,
        pfd::opt::none
    );
    futureSelection_ = task.get_future();
    thread_ = std::thread(std::move(task));
}

bool FileDialog::validSelection()
{
    if (!isOpen_)
        return false;
    auto status = futureSelection_.wait_for(std::chrono::nanoseconds(0));
    if (status != std::future_status::ready)
        return false;
    isOpen_ = false;
    thread_.join();
    selection_ = futureSelection_.get();
    return selection_.size() > 0;
}

}
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

// A threaded C++ wrapper around portable-file-dialogs (I want to avoid the
// performance costs on the main thread that come with using async, which is 
// what portable-file-dialogs uses at its core). While said library also appears
// to support dialog termination, it seems it does not work properly with
// open/save file dialogs (it works with other components, e.g. notifications),
// so no wrapping around the dialog termination functionality for now
class FileDialog
{
private:
    enum class Type
    {
        OpenFile,
        SaveFile
    };
    bool                                  isOpen_       = false;
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
    // Open a native file dialog for opening a file. In runs in a separate
    // thread. To check if a valid selection has been made, use validSelection().
    // If true, the resulting selected filepath/s can be retrieved in
    // selection(). If false, the dialog is still running.
    //
    // Regarding the filters syntax, it follows the following spec., e.g.,:
    // {
    //      "Some files", "*.txt *.json", 
    //      "Other files", "*.pdf *.yml"
    // }
    void runOpenFileDialog
    (
        const std::string&              title = "Open file",
        const std::vector<std::string>& filters = {"All files", "*"}, 
        const std::string&              defaultPath = ".",
        const bool                      multipleSelection = false
    );
    // Open a native file dialog for saving a file. In runs in a separate
    // thread. To check if a valid selection has been made, use validSelection().
    // If true, the resulting selected filepath/s can be retrieved in
    // selection(). If false, the dialog is still running.
    //
    // Regarding the filters syntax, it follows the following spec., e.g.,:
    // {
    //      "Some files", "*.txt *.json", 
    //      "Other files", "*.pdf *.yml"
    // }
    void runSaveFileDialog
    (
        const std::string&              title = "Save file",
        const std::vector<std::string>& filters = {"All files", "*"}, 
        const std::string&              defaultPath = "."
    );
    bool validSelection();
    const std::vector<std::string>& selelction() const {return selection_;}
};
    
}
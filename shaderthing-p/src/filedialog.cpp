#include "shaderthing-p/include/filedialog.h"
extern "C"
{
#include "thirdparty/nativefiledialog/include/nfd.h"
}
#include "algorithm"
#include <chrono>

namespace ShaderThing
{

FileDialog::~FileDialog()
{
    if (isOpen_)
        thread_.join();
}

void FileDialog::open(const char* filters, const char* defaultPath)
{
    if (isOpen_)
        return;
    isOpen_ = true;
    auto task = std::packaged_task<std::string()>
    (
        [filters, defaultPath]()
        {
            std::string filepath;
            nfdchar_t*  selectedFilePath = nullptr;
            nfdresult_t result = 
                NFD_OpenDialog(filters, defaultPath, &selectedFilePath);
            if (result != NFD_OKAY)
                return std::string();
            filepath = std::string(selectedFilePath);
            free(selectedFilePath);
            return filepath;   
        }
    );
    futureFilepath_ = task.get_future();
    thread_ = std::thread(std::move(task));
}

bool FileDialog::validSelection()
{
    if (!isOpen_)
        return false;
    auto status = futureFilepath_.wait_for(std::chrono::nanoseconds(0));
    if (status != std::future_status::ready)
        return false;
    filepath_ = futureFilepath_.get();
    isOpen_ = false;
    thread_.join();
    return true;
}

}
#pragma once

#include <string>
#include <future>
#include <thread>
#include <vector>

namespace ShaderThing
{

// A non-blocking C++ wrapper around NativeFileDialog (NFD)
class FileDialog
{
private:
    bool                     isOpen_ = false;
    std::string              filepath_;
    std::future<std::string> futureFilepath_;
    std::thread              thread_;
public:
    ~FileDialog();
    void open(const char* filters = nullptr, const char* defaultPath = nullptr);
    bool validSelection();
    std::string filepath() const {return filepath_;}
};
    
}
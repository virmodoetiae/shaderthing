#include "vpch.h"
#include "vhelpers.h"

namespace vir
{

namespace Helpers
{

// Load contents of file at filepath to a target string
void readFileToString(std::string filepath, std::string& target)
{
    std::ifstream ifstream(filepath);
    std::stringstream sstream;
    sstream << ifstream.rdbuf();
    target = sstream.str();
    sstream.str(std::string());
}

}

}
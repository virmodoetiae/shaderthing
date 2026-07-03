#ifndef V_HELPERS_H
#define V_HELPERS_H

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

namespace vir
{

namespace Helpers
{

template<typename T>
typename std::vector<T*>::iterator findInPtrVector
(
    const T* item, 
    std::vector<T*>& items
)
{
    return
        std::find_if
        (
            items.begin(), 
            items.end(), 
            [&](const T* itemi){return (*itemi == *item);} 
        );
}

template<typename T>
typename std::vector<T*>::iterator findInPtrVector
(
    const T& item, 
    std::vector<T*>& items
)
{
    return 
        std::find_if
        (
            items.begin(), 
            items.end(), 
            [&](const T* itemi){return (*itemi == item);} 
        );
}

// Load contents of file at filepath to a target string
void readFileToString(std::string filepath, std::string& target);

}

}

#endif
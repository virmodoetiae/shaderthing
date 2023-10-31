#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32)
#include <windows.h>
#endif

#include "shaderthingapp.h"
#include "vir/include/vir.h"

int main()
{

// Hide console if running on Windows
#if defined(__WIN32__) || defined(WIN32) || defined(_WIN32) || defined(__WIN32)
    FreeConsole();
#endif
    ShaderThing::ShaderThingApp();
    return 0;
    
}

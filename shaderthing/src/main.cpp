#ifdef _WIN32
#include <windows.h>
#endif

#include "shaderthingapp.h"
#include "vir/include/vir.h"

int main()
{

// Hide console if running on Windows
#ifdef _WIN32
    ShowWindow(GetConsoleWindow(), SW_HIDE);
#endif
    ShaderThing::ShaderThingApp();
    return 0;
    
}

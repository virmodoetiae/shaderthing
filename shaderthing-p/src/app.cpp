#include "vir/include/vir.h"
#include "shaderthing-p/include/app.h"
#include "shaderthing-p/include/modules/backend.h"
#include "shaderthing-p/include/modules/frontend.h"

namespace ShaderThing
{

App::App()
{
    // Initialize vir lib
    vir::initialize
    (
        vir::PlatformType::GLFWOpenGL,
        512,
        512,
        "ShaderThing"
    );
    auto window = vir::GlobalPtr<vir::Window>::instance();

    Backend::initialize(*this);

    Frontend::initialize(*this);

    // Main loop
    window->run([this]()
    {
        vir::ImGuiRenderer::run(Frontend::renderAppGUI, this);
        for (auto layer : layers)
        {
            Backend::renderLayerShader(layer, nullptr, true, sharedUniforms);
        }
        Backend::update(*this);
    });
}

}
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/gui.h"
#include "shaderthing/include/eventmanager.h"
#include "shaderthing/include/shaderthing.h"
#include "shaderthing/include/structs.h"
#include "shaderthing/include/typedefs.h"
#include "vir/include/vir.h"

namespace ShaderThing
{

void run()
{
    // Initialize vir lib
    vir::Settings settings = {};
    settings.windowName = "ShaderThing";// - "+project_.filename;
    settings.enableFaceCulling = false;
    vir::initialize(settings);

    // Initialize application
    AppState appState = {};
    initialize(appState);

    // Main loop
    auto window = vir::Window::instance();
    while(window->isOpen())
    {   
        // Render GUI and record actions that should only be applied after
        // GUI rendering (deferred actions, e.g., deleting layers, uniforms, 
        // etc.)
        GUI::renderControlPanel(appState);
        preRenderUpdate(appState);
        auto renderResult = renderShaders(appState, nullptr, 1u);
        postRenderUpdate(appState);

        /*
        processProjectActions();
        renderGui();
        update();

        auto result = Layer::renderShaders
        (
            layers_, 
            exporter_->isRunning() ? exporter_->framebuffer().get() : nullptr, 
            *sharedUniforms_,
            exporter_->isRunning() ? exporter_->nRenderPasses() : 1
        );
        if (exporter_->isRunning() && result.renderPassesComplete)
            exporter_->writeOutput();
        */

        window->update(renderResult.flipWindowBuffer);
    }

}

}
#include "shaderthing/include/corelogic.h"
#include "shaderthing/include/gui.h"
#include "shaderthing/include/oo/eventmanager.h"
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
    AppData appData = {};
    initialize(appData);
    
    // Initialize event manager
    auto eventManager = GPtr<EventManager>(new EventManager(appData));

    // Main loop
    auto window = vir::Window::instance();
    while(window->isOpen())
    {   
        // Render GUI and record actions that should only be applied after
        // GUI rendering (deferred actions, e.g., deleting layers, uniforms, 
        // etc.)
        GUI::renderControlPanel(appData);
        preRenderUpdate(appData);
        auto renderResult = renderShaders(appData, nullptr, 1u);
        postRenderUpdate(appData);

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
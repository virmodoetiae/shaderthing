#pragma once

namespace ShaderThing
{

class App;
class SharedUniforms;
class Layer;

namespace Frontend
{

void initialize(App& app);

void renderAppGUI(App* app);

void renderAppMenuBarGUI(App* app);

void renderLayerSettingsGUI(Layer* layer);

void renderLayerTabBarGUI(std::vector<Layer*>& layers, SharedUniforms& sharedUniforms);

void renderLayerTabGUI(Layer* layer);

}
}
#pragma once

namespace ShaderThing
{

class App;
class Layer;

namespace Frontend
{

void initializeGUI(float*& fontScale);

void renderAppGUI(App* app);

void renderAppMenuBarGUI(App* app);

void renderLayerSettingsGUI(Layer* layer);

void renderLayerTabBarGUI(std::vector<Layer*>& layers);

void renderLayerTabGUI(Layer* layer);

}
}
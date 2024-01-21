#pragma once

#include <vector>

namespace ShaderThing
{

class Layer;
class Uniform;

namespace Backend
{

void addNewLayerTo(std::vector<Layer*>& layers);

void deleteLayerFrom(Layer* layer, std::vector<Layer*>& layers);

void constrainAndSetWindowResolution(glm::ivec2& resolution);

}

}
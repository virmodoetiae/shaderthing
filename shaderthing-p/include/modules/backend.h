#pragma once

#include <string>
#include <vector>
#include "thirdparty/glm/glm.hpp"

namespace vir
{
    class Framebuffer;
    class UniformBuffer;
}

namespace ShaderThing
{

class App;
class Layer;
class Uniform;
class SharedUniforms;

namespace Backend
{

void initialize(App& app);

void initializeSharedUniforms(SharedUniforms& sharedUniforms);

void updateSharedUniforms(SharedUniforms& sharedUniforms);

Layer* createLayerIn(std::vector<Layer*>& layers, SharedUniforms& sharedUniforms);

void deleteLayerFrom(Layer* layer, std::vector<Layer*>& layers);

void constrainAndSetWindowResolution(glm::ivec2& resolution);

const std::string& shaderVersionSource();

const std::string& vertexShaderSource();

std::string assembleFragmentShaderSource
(
    Layer* layer, 
    unsigned int& nHeaderLines
);

bool compileLayerShader(Layer* layer, SharedUniforms& sharedUniforms);

void renderLayerShader(Layer* layer, vir::Framebuffer* target, const bool clearTarget, const SharedUniforms& sharedUniforms);

void update(App& app);

}

}
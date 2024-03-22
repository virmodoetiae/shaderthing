#include "shaderthing/include/random.h"
#include <chrono>

namespace ShaderThing
{

Random::Random() :
generator_(std::chrono::high_resolution_clock::now().time_since_epoch().count())
{}

float Random::generateFloat(float min, float max)
{
    return std::uniform_real_distribution<float>(min, max)(generator_);
}

}
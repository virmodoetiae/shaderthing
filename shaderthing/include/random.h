#pragma once

#include <random>

namespace ShaderThing
{

class Random
{
    std::mt19937_64 generator_;
public:
    Random();
    float generateFloat(float min=0, float max=1);
};

}
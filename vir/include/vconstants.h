#ifndef VCONSTANTS_H
#define VCONSTANTS_H

#include <limits>

namespace vir
{

#define PI 3.141593f
#define TPI 6.283186f
constexpr float TPI_BY_360 = TPI/360.0f;
constexpr float PI_HALF = PI/2.0f;
constexpr double MIN_DOUBLE = std::numeric_limits<double>::min();
constexpr double MAX_DOUBLE = std::numeric_limits<double>::max(); 

}

#endif
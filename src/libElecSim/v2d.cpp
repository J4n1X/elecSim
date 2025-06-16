#include "v2d.h"
#include <cmath>
#include <algorithm>
#include <string>

// Note: Most functions in v2d.h are template functions or constexpr functions
// that should remain in the header file for optimal performance and to avoid
// template instantiation issues. Only a few specific functions can be moved here.

// The mag() function for common types - these are not constexpr due to sqrt
template<>
auto v2d<float>::mag() const { 
    return std::sqrt(x * x + y * y); 
}

template<>
auto v2d<double>::mag() const { 
    return std::sqrt(x * x + y * y); 
}

// The norm() function for common types - depends on mag()
template<>
v2d<float> v2d<float>::norm() const {
    auto r = 1.0f / mag();
    return v2d<float>(x * r, y * r);
}

template<>
v2d<double> v2d<double>::norm() const {
    auto r = 1.0 / mag();
    return v2d<double>(x * r, y * r);
}

// The str() function for common types
template<>
std::string v2d<int>::str() const {
    return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + ")";
}

template<>
std::string v2d<float>::str() const {
    return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + ")";
}

template<>
std::string v2d<double>::str() const {
    return std::string("(") + std::to_string(this->x) + "," + std::to_string(this->y) + ")";
}

// The polar() function for common types - uses atan2
template<>
v2d<float> v2d<float>::polar() const { 
    return v2d<float>(mag(), std::atan2(y, x)); 
}

template<>
v2d<double> v2d<double>::polar() const { 
    return v2d<double>(mag(), std::atan2(y, x)); 
}

// The cart() function for common types - uses sin/cos
template<>
v2d<float> v2d<float>::cart() const {
    return v2d<float>(std::cos(y) * x, std::sin(y) * x);
}

template<>
v2d<double> v2d<double>::cart() const {
    return v2d<double>(std::cos(y) * x, std::sin(y) * x);
}

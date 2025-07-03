#pragma once
#include <algorithm>
#include <array>
#include <cmath>
#include <concepts>
#include <iostream>
#include <string>
#include <type_traits>
#include <format>

// Extracted olc::v_2d so that libelecsim does not have to depend on 
// olcPixelGameEngine
namespace ElecSim {
template <class T>
  requires std::is_arithmetic_v<T>
struct v2d {
  T x = 0;
  T y = 0;

  // Default constructor
  inline constexpr v2d() = default;

  // Specific constructor
  inline constexpr v2d(T _x, T _y) : x(_x), y(_y) {}

  // Copy constructor
  inline constexpr v2d(const v2d& v) = default;

  // Assignment operator
  inline constexpr v2d& operator=(const v2d& v) = default;

  inline constexpr std::array<T, 2> a() const { return std::array<T, 2>{x, y}; }

  // Returns rectangular area of vector
  inline constexpr auto area() const { return x * y; }

  // Returns magnitude of vector
  auto mag() const;

  // Returns magnitude squared of vector (useful for fast comparisons)
  inline constexpr T mag2() const { return x * x + y * y; }

  // Returns normalised version of vector
  v2d norm() const;

  // Returns vector at 90 degrees to this one
  inline constexpr v2d perp() const { return v2d(-y, x); }

  // Rounds both components down
  inline constexpr v2d floor() const {
    return v2d(std::floor(x), std::floor(y));
  }

  // Rounds both components up
  inline constexpr v2d ceil() const {
    return v2d(std::ceil(x), std::ceil(y));
  }

  // Returns 'element-wise' max of this and another vector
  inline constexpr v2d max(const v2d& v) const {
    return v2d(std::max(x, v.x), std::max(y, v.y));
  }

  // Returns 'element-wise' min of this and another vector
  inline constexpr v2d min(const v2d& v) const {
    return v2d(std::min(x, v.x), std::min(y, v.y));
  }

  // Calculates scalar dot product between this and another vector
  inline constexpr auto dot(const v2d& rhs) const {
    return this->x * rhs.x + this->y * rhs.y;
  }

  // Calculates 'scalar' cross product between this and another vector (useful
  // for winding orders)
  inline constexpr auto cross(const v2d& rhs) const {
    return this->x * rhs.y - this->y * rhs.x;
  }

  // Treat this as polar coordinate (R, Theta), return cartesian equivalent (X,
  // Y)
  v2d cart() const;

  // Treat this as cartesian coordinate (X, Y), return polar equivalent (R,
  // Theta)
  v2d polar() const;

  // Clamp the components of this vector in between the 'element-wise' minimum
  // and maximum of 2 other vectors
  inline constexpr v2d clamp(const v2d& v1, const v2d& v2) const {
    return this->max(v1).min(v2);
  }

  // Linearly interpolate between this vector, and another vector, given
  // normalised parameter 't'
  inline constexpr v2d lerp(const v2d& v1, const double t) const {
    return (*this) * (T(1.0 - t)) + (v1 * T(t));
  }

  // Compare if this vector is numerically equal to another
  inline constexpr bool operator==(const v2d& rhs) const {
    return (this->x == rhs.x && this->y == rhs.y);
  }

  // Compare if this vector is not numerically equal to another
  inline constexpr bool operator!=(const v2d& rhs) const {
    return (this->x != rhs.x || this->y != rhs.y);
  }

  // Return this vector as a std::string, of the form "(x,y)"
  std::string str() const;

  // Assuming this vector is incident, given a normal, return the reflection
  inline constexpr v2d reflect(const v2d& n) const {
    return (*this) - 2.0 * (this->dot(n) * n);
  }

  // Allow 'casting' from other v_2d types
  template <class F>
  inline constexpr operator v2d<F>() const {
    return {static_cast<F>(this->x), static_cast<F>(this->y)};
  }
};

// Multiplication operator overloads between vectors and scalars, and vectors
// and vectors
template <class TL, class TR>
inline constexpr auto operator*(const TL& lhs, const v2d<TR>& rhs) {
  return v2d(lhs * rhs.x, lhs * rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator*(const v2d<TL>& lhs, const TR& rhs) {
  return v2d(lhs.x * rhs, lhs.y * rhs);
}

template <class TL, class TR>
inline constexpr auto operator*(const v2d<TL>& lhs, const v2d<TR>& rhs) {
  return v2d(lhs.x * rhs.x, lhs.y * rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator*=(v2d<TL>& lhs, const TR& rhs) {
  lhs = lhs * rhs;
  return lhs;
}

// Division operator overloads between vectors and scalars, and vectors and
// vectors
template <class TL, class TR>
inline constexpr auto operator/(const TL& lhs, const v2d<TR>& rhs) {
  return v2d(lhs / rhs.x, lhs / rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator/(const v2d<TL>& lhs, const TR& rhs) {
  return v2d(lhs.x / rhs, lhs.y / rhs);
}

template <class TL, class TR>
inline constexpr auto operator/(const v2d<TL>& lhs, const v2d<TR>& rhs) {
  return v2d(lhs.x / rhs.x, lhs.y / rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator/=(v2d<TL>& lhs, const TR& rhs) {
  lhs = lhs / rhs;
  return lhs;
}

// Unary Addition operator (pointless but i like the platinum trophies)
template <class T>
inline constexpr auto operator+(const v2d<T>& lhs) {
  return v2d(+lhs.x, +lhs.y);
}

// Addition operator overloads between vectors and scalars, and vectors and
// vectors
template <class TL, class TR>
inline constexpr auto operator+(const TL& lhs, const v2d<TR>& rhs) {
  return v2d(lhs + rhs.x, lhs + rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator+(const v2d<TL>& lhs, const TR& rhs) {
  return v2d(lhs.x + rhs, lhs.y + rhs);
}

template <class TL, class TR>
inline constexpr auto operator+(const v2d<TL>& lhs, const v2d<TR>& rhs) {
  return v2d(lhs.x + rhs.x, lhs.y + rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator+=(v2d<TL>& lhs, const TR& rhs) {
  lhs = lhs + rhs;
  return lhs;
}

template <class TL, class TR>
inline constexpr auto operator+=(v2d<TL>& lhs, const v2d<TR>& rhs) {
  lhs = lhs + rhs;
  return lhs;
}

// Unary negation operator overoad for inverting a vector
template <class T>
inline constexpr auto operator-(const v2d<T>& lhs) {
  return v2d(-lhs.x, -lhs.y);
}

// Subtraction operator overloads between vectors and scalars, and vectors and
// vectors
template <class TL, class TR>
inline constexpr auto operator-(const TL& lhs, const v2d<TR>& rhs) {
  return v2d(lhs - rhs.x, lhs - rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator-(const v2d<TL>& lhs, const TR& rhs) {
  return v2d(lhs.x - rhs, lhs.y - rhs);
}

template <class TL, class TR>
inline constexpr auto operator-(const v2d<TL>& lhs, const v2d<TR>& rhs) {
  return v2d(lhs.x - rhs.x, lhs.y - rhs.y);
}

template <class TL, class TR>
inline constexpr auto operator-=(v2d<TL>& lhs, const TR& rhs) {
  lhs = lhs - rhs;
  return lhs;
}

// Greater/Less-Than Operator overloads - mathematically useless, but handy for
// "sorted" container storage
template <class TL, class TR>
inline constexpr bool operator<(const v2d<TL>& lhs, const v2d<TR>& rhs) {
  return (lhs.y < rhs.y) || (lhs.y == rhs.y && lhs.x < rhs.x);
}

template <class TL, class TR>
inline constexpr bool operator>(const v2d<TL>& lhs, const v2d<TR>& rhs) {
  return (lhs.y > rhs.y) || (lhs.y == rhs.y && lhs.x > rhs.x);
}

// Allow olc::v_2d to play nicely with std::cout
template <class T>
inline constexpr std::ostream& operator<<(std::ostream& os,
                                          const v2d<T>& rhs) {
  os << rhs.str();
  return os;
}

typedef v2d<float> vf2d;
typedef v2d<int> vi2d;
typedef v2d<unsigned int> vu2d;
}  // namespace ElecSim

template <typename T>
struct std::formatter<ElecSim::v2d<T>, char> {
  int decimals = 0;  // Number of decimal places to format

  constexpr std::format_parse_context::iterator parse(
      std::format_parse_context& ctx) {
    auto it = ctx.begin();
    auto end = ctx.end();
    // Check for an opening brace for format specifiers
    if (it != end && *it == ':') {
      ++it;  // Consume the ':'
      // If we have a number here, that's the number of decimals
      while (it != end && std::isdigit(*it)) {
        decimals = decimals * 10 + (*it - '0');
        ++it;
      }
      // Loop until a '}' or end of string, consuming characters
      while (it != end && *it != '}') {
        ++it;
      }
    }

    // Return the iterator to the end of the format specifier.
    return it;
  }
  std::format_context::iterator format(const ElecSim::v2d<T>& vec,
                                       std::format_context& ctx) const {
    if constexpr (std::is_floating_point_v<T>) {
      if (decimals >= 0) {
        return std::format_to(ctx.out(), "({:.{}f}, {:.{}f})", vec.x, decimals,
                              vec.y, decimals);
      }
    } else {
      return std::format_to(ctx.out(), "({}, {})", vec.x, vec.y);
    }
  }
};
#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

#include "Common.h"
#include "GridTileTypes.h"
#include "olcPixelGameEngine.h"
#include "v2d.h"

namespace Engine {

template <typename Renderer>
class Drawable {
 public:
  virtual ~Drawable() = default;
  virtual void Draw(Renderer *renderer, vf2d pos, float scale, uint8_t alpha) const = 0;
};



}  // namespace Engine
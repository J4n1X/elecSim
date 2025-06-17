#pragma once

#include <concepts>
#include <memory>
#include <type_traits>

#include "Common.h"
#include "GridTileTypes.h"
#include "RenderManager.h"
#include "olcPixelGameEngine.h"
#include "v2d.h"

namespace Engine {

namespace Target {
namespace Olc {

extern const std::array<olc::Pixel, 7> activeColors;
extern const std::array<olc::Pixel, 7> inactiveColors;

void OlcDrawTile(olc::PixelGameEngine *renderer, olc::vf2d pos, float size,
                 int alpha, bool activation, ElecSim::Direction facing,
                 int tileId);

template <typename T>
  requires std::derived_from<T, ElecSim::GridTile>
class BasicTileDrawable : public Drawable<olc::PixelGameEngine> {
  protected:
  std::shared_ptr<T> tilePtr;

 public:
  BasicTileDrawable() : tilePtr(nullptr) {}
  BasicTileDrawable(std::shared_ptr<T> &&tilePtr) : tilePtr(tilePtr) {}
  void Draw(olc::PixelGameEngine *renderer, vf2d pos, float screenSize,
            uint8_t alpha) const {
    // Convert v2d to olc::vf2d
    olc::vf2d olcPos(pos.x, pos.y);
    OlcDrawTile(renderer, olcPos, screenSize, alpha, tilePtr->GetActivation(),
                tilePtr->GetFacing(), tilePtr->GetTileTypeId());
  }
};

class WireDrawable : public BasicTileDrawable<ElecSim::WireGridTile> {};

class JunctionDrawable : public BasicTileDrawable<ElecSim::JunctionGridTile> {};

class EmitterDrawable : public BasicTileDrawable<ElecSim::EmitterGridTile> {};

class SemiConductorDrawable
    : public BasicTileDrawable<ElecSim::SemiConductorGridTile> {};

class ButtonDrawable : public BasicTileDrawable<ElecSim::ButtonGridTile> {};

class InverterDrawable : public BasicTileDrawable<ElecSim::InverterGridTile> {};

class CrossingDrawable : public BasicTileDrawable<ElecSim::CrossingGridTile> {
 public:
  void Draw(olc::PixelGameEngine *renderer, vf2d pos, float scale,
            uint8_t alpha) const final override;
};
}  // namespace Olc
}  // namespace Target
}  // namespace Engine
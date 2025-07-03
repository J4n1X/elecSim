#include "TileChunk.h"

#include <stdexcept>

#include "Common.h"

namespace Engine {

TileChunk::TileChunk(sf::Vector2f worldPos, const sf::Texture* atlas)
    : texture(atlas) {
  setPosition(worldPos);

}

void TileChunk::SetTile(const ElecSim::GridTile* tile,
                        sf::IntRect textureRect) {
  const auto tilePos = tile->GetPos();

  // Ensure positive modulo result for negative coordinates
  const sf::Vector2f localPos(tilePos.x % CHUNK_LENGTH,
                              tilePos.y % CHUNK_LENGTH);

  const size_t baseIndex = localPos.y * CHUNK_LENGTH * VERTICES_PER_TILE +
                           localPos.x * VERTICES_PER_TILE;
  const sf::Vector2f textureTopLeft(textureRect.position);
  const sf::Vector2f textureBottomRight(textureTopLeft +
                                        sf::Vector2f(textureRect.size));
  const sf::Vector2f textureTopRight(textureBottomRight.x, textureTopLeft.y);
  const sf::Vector2f textureBottomLeft(textureTopLeft.x, textureBottomRight.y);
  sf::Transform transform;
  transform.rotate(sf::degrees(static_cast<float>(tile->GetFacing()) * 90.f),
                   sf::Vector2f(0.5f, 0.5f));  // Rotate around the center

  // Map positions for both triangles using the same transform and offset logic
  vArray[baseIndex + 0].position =
      localPos + transform.transformPoint({0.f, 0.f});
  vArray[baseIndex + 0].texCoords = textureTopLeft;
  vArray[baseIndex + 1].position =
      localPos + transform.transformPoint({1.f, 0.f});
  vArray[baseIndex + 1].texCoords = textureTopRight;
  vArray[baseIndex + 2].position =
      localPos + transform.transformPoint({0.f, 1.f});
  vArray[baseIndex + 2].texCoords = textureBottomLeft;

  vArray[baseIndex + 3].position =
      localPos + transform.transformPoint({1.f, 0.f});
  vArray[baseIndex + 3].texCoords = textureTopRight;
  vArray[baseIndex + 4].position =
      localPos + transform.transformPoint({1.f, 1.f});
  vArray[baseIndex + 4].texCoords = textureBottomRight;
  vArray[baseIndex + 5].position =
      localPos + transform.transformPoint({0.f, 1.f});
  vArray[baseIndex + 5].texCoords = textureBottomLeft;
}

void TileChunk::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= getTransform();
  states.texture = texture;
  target.draw(vArray, states);
}

void TileChunk::EraseTile(const ElecSim::vi2d& tilePos) {
  // Ensure positive modulo result for negative coordinates
  const sf::Vector2f localPos(
      (tilePos.x % CHUNK_LENGTH + CHUNK_LENGTH) % CHUNK_LENGTH,
      (tilePos.y % CHUNK_LENGTH + CHUNK_LENGTH) % CHUNK_LENGTH);
  
  // Calculate the base index for the vertices
  const size_t baseIndex = static_cast<size_t>(localPos.y) * CHUNK_LENGTH * VERTICES_PER_TILE +
                           static_cast<size_t>(localPos.x) * VERTICES_PER_TILE;
  
  // Clear the vertices by setting them to transparent
  for (size_t i = 0; i < VERTICES_PER_TILE; ++i) {
    vArray[baseIndex + i].color = sf::Color::Transparent;
  }
}

}  // namespace Engine
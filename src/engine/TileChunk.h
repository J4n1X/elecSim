#pragma once

#include "GridTile.h"
#include "SFML/Graphics.hpp"

namespace Engine {

class TileChunk : public sf::Drawable, public sf::Transformable {
 public:
  constexpr static size_t CHUNK_SIZE = 16;
  TileChunk() : vArray(sf::PrimitiveType::Triangles) {}
  void AppendVertices(const sf::Vertex* vertices, size_t vertexCount);
  void AppendVertex(const sf::Vertex& vertex) {
    AppendVertices(&vertex, 1);
  }
  void AppendVertexArray(const sf::VertexArray& vertexArray) {
    // I hate that I have to do this, but SFML sucks in this regard.
    const sf::Vertex* vertices = &(vertexArray[0]);
    AppendVertices(vertices, vertexArray.getVertexCount());
  }

 private:
  sf::VertexArray vArray;
  void draw(sf::RenderTarget& target,
            sf::RenderStates states) const final override {
    states.transform *= getTransform();
    target.draw(vArray, states);
  };
};
}  // namespace Engine
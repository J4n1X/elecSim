#include "TileChunk.h"
#include <stdexcept>


namespace Engine {
void TileChunk::AppendVertices(const sf::Vertex *vertices, size_t vertexCount) {
  size_t baseOffset = vArray.getVertexCount();
  vArray.resize(vArray.getVertexCount() + vertexCount);
  // Memcopy it over
  std::copy(vertices, vertices + vertexCount, &(vArray[baseOffset]));
}
}  // namespace Engine
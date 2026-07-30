#include "TileChunk.h"

#include <algorithm>

#include "Common.h"

namespace Engine {

TileChunk::TileChunk(sf::Vector2f worldPos, const sf::Texture* atlas)
    : texture(atlas) {
  setPosition(worldPos);
}

sf::Vector2i TileChunk::LocalCoords(const ElecSim::vi2d& tilePos) noexcept {
  constexpr int len = static_cast<int>(CHUNK_LENGTH);
  return sf::Vector2i(((tilePos.x % len) + len) % len,
                      ((tilePos.y % len) + len) % len);
}

std::size_t TileChunk::SlotIndex(sf::Vector2i local) noexcept {
  return static_cast<std::size_t>(local.y) * CHUNK_LENGTH +
         static_cast<std::size_t>(local.x);
}

void TileChunk::MarkDirty(std::size_t slot) const noexcept {
  if (slotIsDirty[slot]) return;
  slotIsDirty[slot] = true;
  dirtySlots.push_back(static_cast<std::uint16_t>(slot));
}

void TileChunk::ClearDirty() const noexcept {
  for (const auto slot : dirtySlots) slotIsDirty[slot] = false;
  dirtySlots.clear();
}

void TileChunk::SetTile(const ElecSim::GridTile* tile,
                        sf::IntRect textureRect) {
  // A lot of the trigonometry in this function can be precomputed, so we do
  // just that.
  static constexpr std::array<std::array<sf::Vector2f, 4>, 4> CORNER_COORDS = {{
    {{{0,0}, {1,0}, {1,1}, {0,1}}},   // facing 0
    {{{1,0}, {1,1}, {0,1}, {0,0}}},   // 90
    {{{1,1}, {0,1}, {0,0}, {1,0}}},   // 180
    {{{0,1}, {0,0}, {1,0}, {1,1}}},   // 270
  }};
  static_assert(VERTICES_PER_TILE == 6);
  static constexpr std::array<std::size_t, VERTICES_PER_TILE> TRIANGLE_ORDER = 
    {0, 1, 3, 1, 2, 3};


  const sf::Vector2i local = LocalCoords(tile->GetPos());
  const std::size_t slot = SlotIndex(local);
  const std::size_t baseIndex = slot * VERTICES_PER_TILE;

  const sf::Vector2f localPos(static_cast<float>(local.x),
                              static_cast<float>(local.y));

  const sf::Vector2f textureTopLeft(textureRect.position);
  const sf::Vector2f textureBottomRight(textureTopLeft +
                                        sf::Vector2f(textureRect.size));
  const std::array<sf::Vector2f, 4> texCoords = {
      textureTopLeft,
      sf::Vector2f(textureBottomRight.x, textureTopLeft.y),
      textureBottomRight,
      sf::Vector2f(textureTopLeft.x, textureBottomRight.y),
  };

  const auto corners = 
    CORNER_COORDS[static_cast<std::size_t>(tile->GetFacing())];

  for(std::size_t i = 0; i < VERTICES_PER_TILE; i++) {
    const std::size_t c = TRIANGLE_ORDER[i];
    sf::Vertex& v = vertices[baseIndex + i];
    v.position = localPos + corners[c];
    v.texCoords = texCoords[c];
    v.color = sf::Color::White;
  }

  MarkDirty(slot);
}

void TileChunk::EraseTile(const ElecSim::vi2d& tilePos) {
  const std::size_t slot = SlotIndex(LocalCoords(tilePos));
  const std::size_t baseIndex = slot * VERTICES_PER_TILE;

  for (std::size_t i = 0; i < VERTICES_PER_TILE; ++i) {
    vertices[baseIndex + i].color = sf::Color::Transparent;
  }

  MarkDirty(slot);
}

void TileChunk::Sync() const {
  // Nothing pending and the buffer already exists.
  if (dirtySlots.empty() && buffer) return;

  if (!sf::VertexBuffer::isAvailable()) {
    // No GPU buffer support: draw() streams the CPU array instead. Drop the
    // pending list so it does not grow without bound.
    ClearDirty();
    return;
  }

  if (!buffer) {
    auto fresh = std::make_unique<sf::VertexBuffer>(
        sf::PrimitiveType::Triangles, sf::VertexBuffer::Usage::Dynamic);
    if (!fresh->create(VERTEX_COUNT)) return;  // retry next frame
    if (!fresh->update(vertices.data())) return;
    buffer = std::move(fresh);
    ClearDirty();  // the full upload covered everything
    return;
  }

  // Past a certain threshold, submitting individual tile updates becomes too
  // expensive compared to a full chunk upload. Currently based on the size
  // of the chunk, but could be worth making static. 
  constexpr std::size_t FULL_UPLOAD_THRESHOLD = CHUNK_TILE_COUNT / 16; 

  if(dirtySlots.size() > FULL_UPLOAD_THRESHOLD) {
    if(!buffer->update(vertices.data())) return;
    ClearDirty();
    return;
  }

  // Coalesce dirty tiles into contiguous runs so a row of tiles that changed
  // together becomes one upload rather than one per tile.
  std::ranges::sort(dirtySlots);

  std::size_t i = 0;
  while (i < dirtySlots.size()) {
    const std::size_t runStart = dirtySlots[i];
    std::size_t runEnd = runStart + 1;
    while (i + 1 < dirtySlots.size() && dirtySlots[i + 1] == runEnd) {
      ++i;
      ++runEnd;
    }
    ++i;

    const std::size_t firstVertex = runStart * VERTICES_PER_TILE;
    const std::size_t count = (runEnd - runStart) * VERTICES_PER_TILE;
    if (!buffer->update(vertices.data() + firstVertex, count,
                        static_cast<unsigned int>(firstVertex))) {
      return;  // leave the list dirty and try again next frame
    }
  }

  ClearDirty();
}

void TileChunk::draw(sf::RenderTarget& target, sf::RenderStates states) const {
  states.transform *= getTransform();
  states.texture = texture;

  Sync();

  if (buffer) {
    target.draw(*buffer, states);
  } else {
    target.draw(vertices.data(), vertices.size(),
                sf::PrimitiveType::Triangles, states);
  }
}

}  // namespace Engine
#include <string>

#include "Drawables.h"
#include "Game.h"
#include "Grid.h"
#include "SFML/Graphics.hpp"
#include "v2d.h"
#include "Common.h"

#ifdef DEBUG
#include "meshes.h"

int generate_texture_atlas() {
constexpr static uint32_t tileSize = 64;  // Size of each tile in pixels
Engine::TileTextureAtlas textureAtlas(tileSize);
// Save the texture to file
sf::Texture atlasTexture = textureAtlas.GetTexture();
sf::Image atlasImage = atlasTexture.copyToImage();

int atlasWidth = atlasImage.getSize().x;
int atlasHeight = atlasImage.getSize().y;

if (atlasImage.saveToFile("tile_atlas.png")) {
  DebugPrint("Texture atlas saved as 'tile_atlas.png'");
  DebugPrint("Atlas size: {}x{} pixels", atlasWidth, atlasHeight);
  DebugPrint("Tile size: {}x{} pixels", tileSize, tileSize);
} else {
  DebugPrint("Failed to save texture atlas!");
  return -1;
}

return 0;
}
#endif

int main(int argc, char* argv[]) {
#ifdef DEBUG
  // Check if user wants to generate atlas
  if (argc > 1 && std::string(argv[1]) == "--generate-atlas") {
    return generate_texture_atlas();
  }
#endif
  Engine::Game game;
  return game.Run(argc, argv);
}
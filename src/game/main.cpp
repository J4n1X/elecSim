#include <iostream>
#include <string>

#include "Drawables.h"
#include "Game.h"
#include "Grid.h"
#include "SFML/Graphics.hpp"
#include "v2d.h"

#ifdef DEBUG
#include "meshes.h"
#include "ArrowTile_mesh_blob.h"
#include "CrossingTile_mesh_blob.h"
static int generate_texture_atlas() {
  std::cout << "Generating texture atlas...\n";

// Load meshes
  Engine::MeshLoader meshLoader;
  meshLoader.LoadMeshFromString(std::string(reinterpret_cast<const char*>(ArrowTile_mesh_data), ArrowTile_mesh_len));
  meshLoader.LoadMeshFromString(std::string(reinterpret_cast<const char*>(CrossingTile_mesh_data), CrossingTile_mesh_len));

  // Create instances of all tile types
  std::vector<std::shared_ptr<ElecSim::GridTile>> tiles;

  // Wire
  tiles.push_back(std::make_shared<ElecSim::WireGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Junction
  tiles.push_back(std::make_shared<ElecSim::JunctionGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Emitter
  tiles.push_back(std::make_shared<ElecSim::EmitterGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // SemiConductor
  tiles.push_back(std::make_shared<ElecSim::SemiConductorGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Button
  tiles.push_back(std::make_shared<ElecSim::ButtonGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Inverter
  tiles.push_back(std::make_shared<ElecSim::InverterGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Crossing
  tiles.push_back(std::make_shared<ElecSim::CrossingGridTile>(
      vi2d(0, 0), ElecSim::Direction::Top));

  // Create a render texture for the atlas
  const int tileSize = 64;  // Size of each tile in pixels
  const int tilesPerRow = 4;

  const int atlasWidth = tileSize * 2 * tilesPerRow;
  const int atlasHeight = tileSize * (static_cast<int>(tiles.size()) / tilesPerRow * 2);

  sf::RenderTexture renderTexture;
  if (!renderTexture.resize({static_cast<unsigned int>(atlasWidth),
                             static_cast<unsigned int>(atlasHeight)})) {
    std::cerr << "Failed to create render texture!\n";
    return -1;
  }

  // set the proper, zoomed-in view for rendering.
  renderTexture.setView(sf::View(
      sf::FloatRect({0.f, 0.f}, {static_cast<float>(atlasWidth / tileSize),
                                 static_cast<float>(atlasHeight / tileSize)})));

  // Clear the render texture with a transparent background
  renderTexture.clear(sf::Color::Transparent);

  // Create drawables and render each tile to the atlas
  for (size_t i = 0; i < tiles.size(); ++i) {
    auto drawable = Engine::CreateTileRenderable(tiles[i], meshLoader.GetMeshes());

    // Calculate position in the atlas grid
    int row = static_cast<int>(i / tilesPerRow);
    int col = static_cast<int>(i % tilesPerRow);
    float x = col * 2;
    float y = row;

    // Position the drawable
    drawable->setPosition(sf::Vector2f(x, y) + drawable->getOrigin());

    // Update the visual state and draw
    drawable->GetTile()->SetActivation(false);
    renderTexture.draw(*drawable);

    // And once more for the activated state
    drawable->GetTile()->SetActivation(true);
    drawable->setPosition(sf::Vector2f(x + 1.f, y) + drawable->getOrigin());
    renderTexture.draw(*drawable);

    std::cout << "Rendered " << ElecSim::TileTypeToString(tiles[i]->GetTileType()) << " at (" << x << ", "
              << y << ")\n";
  }

  // Finish rendering
  renderTexture.display();

  // Save the texture to file
  sf::Texture atlasTexture = renderTexture.getTexture();
  sf::Image atlasImage = atlasTexture.copyToImage();

  if (atlasImage.saveToFile("tile_atlas.png")) {
    std::cout << "Texture atlas saved as 'tile_atlas.png'\n";
    std::cout << "Atlas size: " << atlasWidth << "x" << atlasHeight << " pixels\n";
    std::cout << "Tile size: " << tileSize << "x" << tileSize << " pixels\n";
    std::cout << "Tiles per row: " << tilesPerRow << "\n";

    // Print tile mapping for reference
    std::cout << "\nTile mapping:\n";
    for (size_t i = 0; i < tiles.size(); ++i) {
      int row = static_cast<int>(i / tilesPerRow);
      int col = static_cast<int>(i % tilesPerRow);
      std::cout << "  " << ElecSim::TileTypeToString(tiles[i]->GetTileType()) << ": (" << col << ", "
                << row << ")\n";
    }

  } else {
    std::cerr << "Failed to save texture atlas!\n";
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
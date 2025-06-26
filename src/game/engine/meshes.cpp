#include "meshes.h"
#include <stdexcept>
#include <fstream>

namespace Engine {

// Move the non-constexpr, non-template runtime implementations here
std::vector<sf::Vertex> TileModel::GetVertices(sf::Transform transform,
                                               bool activated) const {
  const sf::Color& realActiveColor = colors[activated];
  const sf::Color& realInactiveColor = colors[!activated];

  std::vector<sf::Vertex> result;
  result.reserve(triangles_count * 3);

  for (size_t i = 0; i < triangles_count; ++i) {
    const auto& triangle = triangles_data[i];
    result.emplace_back(transform.transformPoint(triangle[0]),
                        activated ? realActiveColor : realInactiveColor);
    result.emplace_back(transform.transformPoint(triangle[1]),
                        activated ? realActiveColor : realInactiveColor);
    result.emplace_back(transform.transformPoint(triangle[2]),
                        activated ? realActiveColor : realInactiveColor);
  }

  return result;
}

void MeshLoader::LoadMeshFromFile(
    const std::filesystem::path& filePath) {
  // Open the file
  std::ifstream fileStream(filePath);
  if (!fileStream.is_open()) {
    throw std::runtime_error("Could not open file: " + filePath.string());
  }
  // Parse the mesh from the stream
  auto vertexArrays = ParseMeshFromStream(fileStream);
  fileStream.close();
  
  meshes.assign(vertexArrays.begin(), vertexArrays.end());
}

std::vector<sf::VertexArray> MeshLoader::ParseMeshFromStream(
    std::istream& stream) {
  std::vector<sf::VertexArray> result;
  std::string line;

  // Parse primitive type
  if (!std::getline(stream, line)) {
    throw std::runtime_error("Empty input stream - no primitive type found");
  }

  std::istringstream typeStream(line);
  std::string typeKeyword;
  std::string primitiveTypeStr;

  // Skip over comments (if there are any) at the start of the line
  while (typeStream.peek() == '#') {
    std::getline(typeStream, line);
    if (!std::getline(stream, line)) {
      throw std::runtime_error("Empty input stream - no primitive type found");
    }
    typeStream.clear();
    typeStream.str(line);
  }

  if (!(typeStream >> typeKeyword >> primitiveTypeStr) ||
      typeKeyword != "type") {
    throw std::runtime_error("Invalid format: expected 'type <PrimitiveType>' on first line");
  }

  sf::PrimitiveType primitiveType;
  if (primitiveTypeStr == "Points") {
    primitiveType = sf::PrimitiveType::Points;
  } else if (primitiveTypeStr == "Lines") {
    primitiveType = sf::PrimitiveType::Lines;
  } else if (primitiveTypeStr == "LineStrip") {
    primitiveType = sf::PrimitiveType::LineStrip;
  } else if (primitiveTypeStr == "Triangles") {
    primitiveType = sf::PrimitiveType::Triangles;
  } else if (primitiveTypeStr == "TriangleStrip") {
    primitiveType = sf::PrimitiveType::TriangleStrip;
  } else if (primitiveTypeStr == "TriangleFan") {
    primitiveType = sf::PrimitiveType::TriangleFan;
  } else {
    throw std::runtime_error("Invalid primitive type: " + primitiveTypeStr);
  }

  std::vector<std::vector<sf::Vertex>> vertexArrays;
  size_t colorCount = 0;
  bool firstVertex = true;
  int lineNumber = 1;

  // Parse vertices
  while (std::getline(stream, line)) {
    ++lineNumber;
    std::istringstream vertexStream(line);
    std::string token;

    if (!(vertexStream >> token)) {
      continue;  // Skip empty lines
    }
    
    // Skip comment lines that begin with '#'
    if (token[0] == '#') {
      continue;
    }
    
    if (token != "v") {
      throw std::runtime_error("Invalid line " + std::to_string(lineNumber) + 
                              ": expected 'v' but got '" + token + "'");
    }

    float x, y;
    if (!(vertexStream >> x >> y)) {
      throw std::runtime_error("Invalid line " + std::to_string(lineNumber) + 
                              ": could not parse vertex coordinates");
    }

    if (!(vertexStream >> token) || token != "c") {
      throw std::runtime_error("Invalid line " + std::to_string(lineNumber) + 
                              ": expected 'c' for color data");
    }

    std::vector<sf::Color> colors;
    std::string colorStr;

    // Read rest of line for colors
    std::string remaining;
    std::getline(vertexStream, remaining);
    std::istringstream colorStream(remaining);

    while (std::getline(colorStream, colorStr, '|')) {
      std::istringstream singleColorStream(colorStr);
      int r, g, b, a;
      if (singleColorStream >> r >> g >> b >> a) {
        colors.emplace_back(r, g, b, a);
      }
    }

    if (colors.empty()) {
      throw std::runtime_error("Invalid line " + std::to_string(lineNumber) + 
                              ": no valid colors found after 'c'");
    }

    // Initialize vertex arrays on first vertex
    if (firstVertex) {
      firstVertex = false;
      colorCount = colors.size();
      vertexArrays.resize(colorCount);
    } else if (colors.size() != colorCount) {
      throw std::runtime_error("Invalid line " + std::to_string(lineNumber) + 
                              ": inconsistent color count (expected " + 
                              std::to_string(colorCount) + ", got " + 
                              std::to_string(colors.size()) + ")");
    }

    // Add vertex to each color variant
    for (size_t i = 0; i < colorCount; ++i) {
      vertexArrays[i].emplace_back(sf::Vector2f(x, y), colors[i]);
    }
  }

  // Convert to sf::VertexArray
  for (auto& vertices : vertexArrays) {
    sf::VertexArray array(primitiveType, vertices.size());
    for (size_t i = 0; i < vertices.size(); ++i) {
      array[i] = vertices[i];
    }
    result.push_back(array);
  }

  return result;
}

}  // namespace Engine

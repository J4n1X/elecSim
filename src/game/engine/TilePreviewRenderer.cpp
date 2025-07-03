#include "TilePreviewRenderer.h"
#include "Common.h"
#include <functional>

namespace Engine {

TilePreviewRenderer::TilePreviewRenderer()
    : sf::Transformable(), // Initialize the transformable part
      previewAlpha(128), // 50% transparent by default
      shadersAvailable(false) {
}

TilePreviewRenderer::~TilePreviewRenderer() {
    // Shader is managed by std::unique_ptr
}

void TilePreviewRenderer::Initialize() {
    previewChunkManager = TileChunkManager();
    shadersAvailable = InitializeShader();
    
    // Log important initialization information and error conditions only
    
    if (!shadersAvailable) {
        DebugPrint("TilePreviewRenderer: Shaders not available, transparency effects disabled");
    }
}

bool TilePreviewRenderer::InitializeShader() {
    // Check if shaders are generally available
    if (!sf::Shader::isAvailable()) {
        return false;
    }
    
    // Create the alpha shader
    alphaShader = std::make_unique<sf::Shader>();
    
    // Load the shader program
    bool success = alphaShader->loadFromMemory(
        // Simple vertex shader that passes through the position and texture coordinates
        "void main() {"
        "    gl_Position = gl_ModelViewProjectionMatrix * gl_Vertex;"
        "    gl_TexCoord[0] = gl_TextureMatrix[0] * gl_MultiTexCoord0;"
        "    gl_FrontColor = gl_Color;"
        "}",
        // Fragment shader that applies the alpha
        "uniform sampler2D texture;"
        "uniform float alpha;"
        "void main() {"
        "    vec4 pixel = texture2D(texture, gl_TexCoord[0].xy);"
        "    gl_FragColor = vec4(pixel.rgb, pixel.a * alpha);"
        "}"
    );
    
    if (!success) {
        alphaShader.reset();
        return false;
    }
    
    return true;
}

void TilePreviewRenderer::UpdatePreview(
    const std::vector<std::unique_ptr<ElecSim::GridTile>>& tiles,
    const TileTextureAtlas& textureAtlas) {
    
    // Clear previous preview
    ClearPreview();
    
    // No need to do anything if buffer is empty
    if (tiles.empty()) return;
    
    // Add all tiles to the preview chunk manager directly from the original tiles
    for (const auto& tile : tiles) {
        // Add the tile to the chunk manager using the simplified API
        previewChunkManager.SetTile(tile.get(), textureAtlas);
    }
}

void TilePreviewRenderer::ClearPreview() {
    previewChunkManager.clear();
}

void TilePreviewRenderer::draw(sf::RenderTarget& target, sf::RenderStates states) const {
    // Apply the transformation from sf::Transformable
    states.transform *= getTransform();
    
    // Add transparency effect with shader if available
    if (shadersAvailable && alphaShader) {
        states.shader = alphaShader.get();
        alphaShader->setUniform("texture", sf::Shader::CurrentTexture);
        alphaShader->setUniform("alpha", static_cast<float>(previewAlpha) / 255.0f);
    }
    
    // Draw the preview
    target.draw(previewChunkManager, states);
}

bool TilePreviewRenderer::IsTransparencyAvailable() const {
    return shadersAvailable && alphaShader != nullptr;
}

void TilePreviewRenderer::SetPreviewAlpha(uint8_t alpha) {
    previewAlpha = alpha;
}

void TilePreviewRenderer::DebugPrintState() const {
    DebugPrint("TilePreviewRenderer State:");
    DebugPrint("  - Transform Position (world): ({},{})", getPosition().x, getPosition().y);
    DebugPrint("  - Shaders Available: {}", shadersAvailable ? "yes" : "no");
    DebugPrint("  - Alpha Value: {}", static_cast<int>(previewAlpha));
}


} // namespace Engine

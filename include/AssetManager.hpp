#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <string>
#include <unordered_map>
#include <vector>
#include <stdexcept>

// ─── Singleton asset manager ──────────────────────────────────────────────────
// Loads each texture/font once. Also pre-computes per-texture:
//   • sf::Image  — pixel data kept in RAM for per-pixel collision tests
//   • hull       — normalised UV convex-hull polygon of non-transparent pixels
//                  (used in debug mode to draw the exact collision boundary)
class AssetManager {
public:
    static AssetManager& instance();

    AssetManager(const AssetManager&)            = delete;
    AssetManager& operator=(const AssetManager&) = delete;

    void loadAll();

    sf::Font&    font();
    sf::Texture& texture(const std::string& name);

    // CPU-side pixel data for per-pixel collision (never modified after load).
    const sf::Image& image(const std::string& name) const;

    // Normalised UV [0,1] convex hull of the non-transparent region.
    // Points are in counter-clockwise order.  Used to draw debug outlines.
    const std::vector<sf::Vector2f>& hullUV(const std::string& name) const;

    // Alpha threshold: pixels with alpha <= this value are IGNORED.
    static constexpr sf::Uint8 ALPHA_THRESHOLD = 12;

private:
    AssetManager() = default;

    void computePixelData(const std::string& name);

    // Monotone-chain convex hull of a point set (returns CCW polygon).
    static std::vector<sf::Vector2f>
        convexHull(std::vector<sf::Vector2f> pts);

    sf::Font m_font;
    std::unordered_map<std::string, sf::Texture>              m_textures;
    std::unordered_map<std::string, sf::Image>                m_images;
    std::unordered_map<std::string, std::vector<sf::Vector2f>> m_hullUVs;

    static const std::vector<sf::Vector2f> s_emptyHull;
    static const sf::Image                 s_emptyImage;
};

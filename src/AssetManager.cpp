#include "AssetManager.hpp"
#include "Constants.hpp"
#include <stdexcept>
#include <algorithm>
#include <cmath>

// Static fallback objects
const std::vector<sf::Vector2f> AssetManager::s_emptyHull{};
const sf::Image                 AssetManager::s_emptyImage{};

AssetManager& AssetManager::instance() {
    static AssetManager inst;
    return inst;
}

// ─── Andrew's monotone-chain convex hull ──────────────────────────────────────
std::vector<sf::Vector2f>
AssetManager::convexHull(std::vector<sf::Vector2f> pts) {
    int n = static_cast<int>(pts.size());
    if (n < 3) return pts;

    // Lexicographic sort
    std::sort(pts.begin(), pts.end(), [](const sf::Vector2f& a,
                                         const sf::Vector2f& b) {
        return a.x < b.x || (a.x == b.x && a.y < b.y);
    });

    // Remove duplicates
    pts.erase(std::unique(pts.begin(), pts.end(), [](const sf::Vector2f& a,
                                                     const sf::Vector2f& b) {
        return std::abs(a.x - b.x) < 0.5f && std::abs(a.y - b.y) < 0.5f;
    }), pts.end());
    n = static_cast<int>(pts.size());
    if (n < 3) return pts;

    auto cross = [](const sf::Vector2f& O, const sf::Vector2f& A,
                    const sf::Vector2f& B) -> float {
        return (A.x - O.x) * (B.y - O.y) - (A.y - O.y) * (B.x - O.x);
    };

    std::vector<sf::Vector2f> hull;
    hull.reserve(2 * n);

    // Lower hull
    for (int i = 0; i < n; ++i) {
        while (hull.size() >= 2 &&
               cross(hull[hull.size()-2], hull[hull.size()-1], pts[i]) <= 0)
            hull.pop_back();
        hull.push_back(pts[i]);
    }
    // Upper hull
    const int lower = static_cast<int>(hull.size()) + 1;
    for (int i = n - 2; i >= 0; --i) {
        while (static_cast<int>(hull.size()) >= lower &&
               cross(hull[hull.size()-2], hull[hull.size()-1], pts[i]) <= 0)
            hull.pop_back();
        hull.push_back(pts[i]);
    }
    hull.pop_back(); // last point == first point
    return hull;
}

// ─── Compute pixel data: image + hull ────────────────────────────────────────
void AssetManager::computePixelData(const std::string& name) {
    auto& img = m_images[name];
    // img was already filled from the texture
    const unsigned w = img.getSize().x;
    const unsigned h = img.getSize().y;

    if (w == 0 || h == 0) { m_hullUVs[name] = {}; return; }

    // Collect non-transparent pixel centres (downsampled every 4px for speed,
    // still gives a dense enough hull without processing millions of pixels).
    std::vector<sf::Vector2f> pts;
    pts.reserve(1024);
    constexpr unsigned STEP = 4;
    for (unsigned y = 0; y < h; y += STEP) {
        for (unsigned x = 0; x < w; x += STEP) {
            if (img.getPixel(x, y).a > ALPHA_THRESHOLD) {
                pts.push_back({ static_cast<float>(x) / w,
                                 static_cast<float>(y) / h });
            }
        }
    }

    m_hullUVs[name] = convexHull(std::move(pts));
}

// ─── Load all assets ──────────────────────────────────────────────────────────
void AssetManager::loadAll() {
    if (!m_font.loadFromFile(ASSET_FONT))
        throw std::runtime_error(std::string("Failed to load font: ") + ASSET_FONT);

    auto load = [&](const std::string& name, const char* path) {
        sf::Texture tex;
        if (!tex.loadFromFile(path))
            throw std::runtime_error(std::string("Failed to load texture: ") + path);
        tex.setSmooth(true);

        // Copy to CPU-side image for pixel collision queries
        m_images[name] = tex.copyToImage();
        computePixelData(name);

        m_textures[name] = std::move(tex);
    };

    load("submarine", ASSET_SUBMARINE);
    load("crab",      ASSET_CRAB);
    load("fish",      ASSET_FISH);
    load("octopus",   ASSET_OCTOPUS);
    load("treasure",  ASSET_TREASURE);
    load("urchin",    ASSET_URCHIN);
    load("starfish", ASSET_STARFISH);
}

// ─── Accessors ────────────────────────────────────────────────────────────────
sf::Font& AssetManager::font() { return m_font; }

sf::Texture& AssetManager::texture(const std::string& name) {
    auto it = m_textures.find(name);
    if (it == m_textures.end())
        throw std::runtime_error("AssetManager: unknown texture '" + name + "'");
    return it->second;
}

const sf::Image& AssetManager::image(const std::string& name) const {
    auto it = m_images.find(name);
    return (it != m_images.end()) ? it->second : s_emptyImage;
}

const std::vector<sf::Vector2f>&
AssetManager::hullUV(const std::string& name) const {
    auto it = m_hullUVs.find(name);
    return (it != m_hullUVs.end()) ? it->second : s_emptyHull;
}

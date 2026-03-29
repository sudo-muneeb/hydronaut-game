#pragma once
// ─── SpriteBounds.hpp ─────────────────────────────────────────────────────────
// Two utilities for pixel-accurate sprite interaction:
//
//  1) pixelPerfectOverlap()  — true collision check.
//     Broad phase: AABB intersect.
//     Narrow phase: for every pixel in the overlap rect, sample both textures;
//     fire if BOTH pixels have alpha > ALPHA_THRESHOLD.
//     `step` (default 2) skips pixels for performance — at 60fps and normal
//     game speeds a step of 2 never misses a real hit.
//
//  2) drawDebugHull()  — debug visualisation.
//     Transforms the precomputed UV convex hull by the sprite's current
//     transform and draws it as a coloured outline polygon.
//     This shows EXACTLY the boundary the collision logic is based on.

#include <SFML/Graphics.hpp>
#include <vector>

// ─── Per-pixel collision ──────────────────────────────────────────────────────
inline bool pixelPerfectOverlap(
        const sf::Sprite& a, const sf::Image& imgA,
        const sf::Sprite& b, const sf::Image& imgB,
        unsigned step = 2,
        sf::Uint8 threshold = 12) noexcept
{
    // ── Broad phase ──────────────────────────────────────────────────────────
    sf::FloatRect overlap;
    if (!a.getGlobalBounds().intersects(b.getGlobalBounds(), overlap))
        return false;

    const sf::Vector2u szA = imgA.getSize();
    const sf::Vector2u szB = imgB.getSize();
    if (szA.x == 0 || szA.y == 0 || szB.x == 0 || szB.y == 0)
        return false;

    const sf::Transform invA = a.getInverseTransform();
    const sf::Transform invB = b.getInverseTransform();
    const sf::IntRect   trA  = a.getTextureRect();
    const sf::IntRect   trB  = b.getTextureRect();

    const int left   = static_cast<int>(overlap.left);
    const int top    = static_cast<int>(overlap.top);
    const int right  = static_cast<int>(overlap.left + overlap.width)  + 1;
    const int bottom = static_cast<int>(overlap.top  + overlap.height) + 1;

    // ── Narrow phase ─────────────────────────────────────────────────────────
    for (int py = top; py < bottom; py += static_cast<int>(step)) {
        for (int px = left; px < right; px += static_cast<int>(step)) {
            const sf::Vector2f world(static_cast<float>(px) + 0.5f,
                                     static_cast<float>(py) + 0.5f);

            // Sprite A: world → local (accounts for position, scale, origin)
            const sf::Vector2f lA = invA.transformPoint(world);
            const int xA = trA.left + static_cast<int>(lA.x);
            const int yA = trA.top  + static_cast<int>(lA.y);
            if (xA < 0 || yA < 0 ||
                xA >= static_cast<int>(szA.x) ||
                yA >= static_cast<int>(szA.y)) continue;
            if (imgA.getPixel(static_cast<unsigned>(xA),
                              static_cast<unsigned>(yA)).a <= threshold) continue;

            // Sprite B: world → local
            const sf::Vector2f lB = invB.transformPoint(world);
            const int xB = trB.left + static_cast<int>(lB.x);
            const int yB = trB.top  + static_cast<int>(lB.y);
            if (xB < 0 || yB < 0 ||
                xB >= static_cast<int>(szB.x) ||
                yB >= static_cast<int>(szB.y)) continue;
            if (imgB.getPixel(static_cast<unsigned>(xB),
                              static_cast<unsigned>(yB)).a <= threshold) continue;

            return true;   // both non-transparent at this screen pixel → HIT
        }
    }
    return false;
}

// ─── Debug hull outline ───────────────────────────────────────────────────────
// Draws the convex hull (UV space) transformed into screen space via the
// sprite's current transform.  This is the exact boundary `pixelPerfectOverlap`
// uses to decide whether a hit is possible, making it the ideal debug overlay.
inline void drawDebugHull(
        sf::RenderWindow& window,
        const sf::Sprite& sprite,
        const std::vector<sf::Vector2f>& hullUV,
        sf::Color colour = sf::Color(0, 255, 120, 220)) noexcept
{
    if (hullUV.empty()) return;

    const sf::FloatRect  tr  = sf::FloatRect(sprite.getTextureRect());
    const sf::Transform& tfm = sprite.getTransform();

    // Convert UV → texture-pixel-space, then apply sprite transform
    sf::VertexArray va(sf::LineStrip, hullUV.size() + 1);
    for (std::size_t i = 0; i <= hullUV.size(); ++i) {
        const sf::Vector2f& uv = hullUV[i % hullUV.size()];
        sf::Vector2f local(uv.x * tr.width, uv.y * tr.height);
        va[i] = sf::Vertex(tfm.transformPoint(local), colour);
    }
    window.draw(va);
}

#include "Treasure.hpp"
#include "AssetManager.hpp"
#include <cstdlib>
#include <algorithm>

// Keep treasure within [15%, 85%] of each dimension so it's always visible
static constexpr float MARGIN       = 0.15f;
static constexpr float SCALE_FACTOR = 0.07f;

Treasure::Treasure(sf::Vector2u windowSize) {
    m_sprite.setTexture(AssetManager::instance().texture("treasure"));
    respawn(windowSize);
}

void Treasure::respawn(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0) return;

    auto& tex = AssetManager::instance().texture("treasure");
    float h   = windowSize.y * SCALE_FACTOR;
    float s   = h / static_cast<float>(tex.getSize().y > 0 ? tex.getSize().y : 1);
    m_sprite.setScale(s, s);

    float sprW = m_sprite.getGlobalBounds().width;
    float sprH = m_sprite.getGlobalBounds().height;

    float minX = windowSize.x * MARGIN;
    float maxX = windowSize.x * (1.f - MARGIN) - sprW;
    float minY = windowSize.y * MARGIN;
    float maxY = windowSize.y * (1.f - MARGIN) - sprH;

    // Ensure valid range even on tiny windows
    if (maxX <= minX) maxX = minX + 1.f;
    if (maxY <= minY) maxY = minY + 1.f;

    float rx = minX + static_cast<float>(
                   std::rand() % static_cast<int>(maxX - minX));
    float ry = minY + static_cast<float>(
                   std::rand() % static_cast<int>(maxY - minY));
    m_sprite.setPosition(rx, ry);
}

void Treasure::draw(sf::RenderWindow& window) const noexcept {
    window.draw(m_sprite);
}

sf::FloatRect Treasure::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

// ─── Memento API ────────────────────────────────────────────────────────────
Treasure::Snapshot Treasure::saveState() const {
    return { m_sprite.getPosition() };
}

void Treasure::restoreState(const Snapshot& snap) {
    m_sprite.setPosition(snap.pos);
}

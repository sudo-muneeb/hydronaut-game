#include "SecObstacle.hpp"
#include "AssetManager.hpp"
#include <cstdlib>
#include <cmath>


const float SCALE_FACTOR = 0.15f;  // fraction of window height occupied by sec obstacles

SecObstacle::SecObstacle(sf::Vector2u windowSize, const std::string& textureName) {
    m_sprite.setTexture(AssetManager::instance().texture(textureName));
    reset(windowSize);
}

void SecObstacle::reset(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  // 20% of window height
    float rangeX = windowSize.x * 0.5f;
    m_x = windowSize.x * 0.5f +
          (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
    m_sprite.setPosition(m_x, secY(m_x, windowSize));
}

float SecObstacle::secY(float x, sf::Vector2u win) const noexcept {
    float w    = static_cast<float>(win.x);
    float h    = static_cast<float>(win.y);
    float half = h * 0.5f;
    if (w < 1.f) return half;                 // guard: degenerate window
    float angle = (static_cast<float>(M_PI) / w) * 3.f * x;
    float c     = std::cos(angle);
    if (std::abs(c) < 0.05f) c = 0.05f;      // avoid sec() asymptote
    float val   = (1.f / c) / 6.f;
    return (val > 0) ? (val * h * 0.5f)
                     : (h * 0.5f * (1.f - std::abs(val)) + half);
}

void SecObstacle::update(sf::Vector2u windowSize) noexcept {
    if (windowSize.x == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  // re-scale each frame for live resizes
    float prevX   = m_x;
    float prevY   = secY(m_x, windowSize);
    bool  wrapped = false;

    m_x -= 2.f * m_speedMult;
    if (m_x < 0) {
        float rangeX = windowSize.x * 0.5f;
        m_x = windowSize.x * 0.5f +
              (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
        wrapped = true;
    }
    float newY = secY(m_x, windowSize);
    m_sprite.setPosition(m_x, newY);

    m_lastVel = wrapped ? sf::Vector2f{0.f, 0.f}
                        : sf::Vector2f{m_x - prevX, newY - prevY};
}

void SecObstacle::draw(sf::RenderWindow& window) const noexcept {
    window.draw(m_sprite);
}

sf::FloatRect SecObstacle::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

// ─── Memento API ────────────────────────────────────────────────────────────
SecObstacle::Snapshot SecObstacle::saveState() const {
    return { m_x, m_speedMult, m_lastVel };
}

void SecObstacle::restoreState(const Snapshot& snap, sf::Vector2u windowSize) {
    m_x         = snap.x;
    m_speedMult = snap.speedMult;
    m_lastVel   = snap.lastVel;
    m_sprite.setPosition(m_x, secY(m_x, windowSize));
}

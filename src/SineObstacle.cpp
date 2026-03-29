#include "SineObstacle.hpp"
#include "AssetManager.hpp"
#include <cstdlib>
#include <cmath>

// Scale: 20% of window height — delegated to Obstacle::applySpriteScale().

constexpr const float SCALE_FACTOR = 0.15f;  // 20% of window height

SineObstacle::SineObstacle(sf::Vector2u windowSize, const std::string& textureName) {
    m_sprite.setTexture(AssetManager::instance().texture(textureName));
    reset(windowSize);
}

void SineObstacle::reset(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);   
    float rangeX = windowSize.x * 0.4f;
    m_x = windowSize.x * 0.6f +
          (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
    m_sprite.setPosition(m_x, sineY(m_x, windowSize));
}

float SineObstacle::sineY(float x, sf::Vector2u win) const noexcept {
    float w = static_cast<float>(win.x);
    float h = static_cast<float>(win.y);
    float half = h * 0.5f;
    if (x <= w * 0.5f)
        return half * (1.f - std::sin(M_PI / (w * 0.5f) * x));
    else
        return half + std::abs(std::sin(M_PI / (w * 0.5f) * (x - w * 0.5f))) * half;
}

void SineObstacle::update(sf::Vector2u windowSize) noexcept {
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  // re-scale each frame for live resizes
    float prevX = m_x;
    float prevY = sineY(m_x, windowSize);

    m_x -= 4.5f * m_speedMult;
    if (m_x < 0 && windowSize.x > 0) {
        float rangeX = windowSize.x * 0.5f;
        m_x = windowSize.x * 0.5f +
              (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
    }
    float newY = sineY(m_x, windowSize);
    m_sprite.setPosition(m_x, newY);

    // RL: record per-frame displacement so agent can infer direction + speed
    m_lastVel = { m_x - prevX, newY - prevY };
}

void SineObstacle::draw(sf::RenderWindow& window) const noexcept {
    window.draw(m_sprite);
}

sf::FloatRect SineObstacle::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

// ─── Memento API ────────────────────────────────────────────────────────────
SineObstacle::Snapshot SineObstacle::saveState() const {
    return { m_x, m_speedMult, m_lastVel };
}

void SineObstacle::restoreState(const Snapshot& snap, sf::Vector2u windowSize) {
    m_x         = snap.x;
    m_speedMult = snap.speedMult;
    m_lastVel   = snap.lastVel;
    m_sprite.setPosition(m_x, sineY(m_x, windowSize));
}

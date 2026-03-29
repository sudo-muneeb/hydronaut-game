#include "ExpSineObstacle.hpp"
#include "AssetManager.hpp"
#include <cstdlib>
#include <cmath>

const float SCALE_FACTOR = 0.15f;  // fraction of window height occupied by exp-sine obstacles


ExpSineObstacle::ExpSineObstacle(sf::Vector2u windowSize, const std::string& textureName) {
    m_sprite.setTexture(AssetManager::instance().texture(textureName));
    reset(windowSize);
}

void ExpSineObstacle::reset(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  // 20% of window height
    float rangeX = windowSize.x * 0.5f;
    m_x = windowSize.x * 0.5f +
          (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
    m_sprite.setPosition(m_x, expSineY(m_x, windowSize));
}

float ExpSineObstacle::expSineY(float x, sf::Vector2u win) const noexcept {
    float h         = static_cast<float>(win.y);
    float half      = h * 0.5f;
    float amplitude = h * 0.15f;  // 15% of window height
    // exp(-0.001*x) naturally decays to ~0 for large x; no special guard needed.
    return half + amplitude * std::sin(0.01f * x) * std::exp(-0.001f * x);
}

void ExpSineObstacle::update(sf::Vector2u windowSize) noexcept {
    if (windowSize.x == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  
    float prevX   = m_x;
    float prevY   = expSineY(m_x, windowSize);
    bool  wrapped = false;

    m_x -= 4.5f * m_speedMult;
    if (m_x < 0) {
        float rangeX = windowSize.x * 0.5f;
        m_x = windowSize.x * 0.5f +
              (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
        wrapped = true;
    }
    float newY = expSineY(m_x, windowSize);
    m_sprite.setPosition(m_x, newY);

    m_lastVel = wrapped ? sf::Vector2f{0.f, 0.f}
                        : sf::Vector2f{m_x - prevX, newY - prevY};
}

void ExpSineObstacle::draw(sf::RenderWindow& window) const noexcept {
    window.draw(m_sprite);
}

sf::FloatRect ExpSineObstacle::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

// ─── Memento API ────────────────────────────────────────────────────────────
ExpSineObstacle::Snapshot ExpSineObstacle::saveState() const {
    return { m_x, m_speedMult, m_lastVel };
}

void ExpSineObstacle::restoreState(const Snapshot& snap, sf::Vector2u windowSize) {
    m_x         = snap.x;
    m_speedMult = snap.speedMult;
    m_lastVel   = snap.lastVel;
    m_sprite.setPosition(m_x, expSineY(m_x, windowSize));
}

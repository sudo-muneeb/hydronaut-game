#include "ParabolicObstacle.hpp"
#include "AssetManager.hpp"
#include <cstdlib>
#include <cmath>

const float SCALE_FACTOR = 0.15f;  // fraction of window height occupied by parabolic obstacles

ParabolicObstacle::ParabolicObstacle(sf::Vector2u windowSize, const std::string& textureName)
    : m_movingLeft(true)
{
    m_sprite.setTexture(AssetManager::instance().texture(textureName));
    reset(windowSize);
}

void ParabolicObstacle::reset(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  
    float rangeX = windowSize.x * 0.5f;
    m_x = windowSize.x * 0.5f +
          (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
    m_movingLeft = true;
    m_sprite.setPosition(m_x, parabolicY(m_x, windowSize));
}

float ParabolicObstacle::parabolicY(float x, sf::Vector2u win) const noexcept {
    float w    = static_cast<float>(win.x);
    float h    = static_cast<float>(win.y);
    float half = h * 0.5f;
    // Parabola scaled so that at x=0 and x=w the value is ~half ± half
    float norm = x / w;   // 0..1
    float para = 4.f * norm * (1.f - norm);  // peaks at 1 when norm=0.5
    if (m_movingLeft)
        return half - half * std::sqrt(std::max(0.f, para));
    else
        return half + half * std::sqrt(std::max(0.f, para));
}

void ParabolicObstacle::update(sf::Vector2u windowSize) noexcept {
    if (windowSize.x == 0) return;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);  
    float prevX = m_x;
    float prevY = parabolicY(m_x, windowSize);
    bool  wrapped = false;

    float speed = 6.f * m_speedMult;
    if (m_movingLeft) {
        m_x -= speed;
        if (m_x < 0) m_movingLeft = false;
    } else {
        m_x += speed;
        if (m_x > static_cast<float>(windowSize.x)) {
            m_movingLeft = true;
            float rangeX = windowSize.x * 0.5f;
            m_x = windowSize.x * 0.5f +
                  (rangeX > 0 ? static_cast<float>(std::rand() % static_cast<int>(rangeX)) : 0.f);
            wrapped = true;
        }
    }
    float newY = parabolicY(m_x, windowSize);
    m_sprite.setPosition(m_x, newY);

    // RL: record displacement; zero on wrap to avoid ghost velocity spike
    m_lastVel = wrapped ? sf::Vector2f{0.f, 0.f}
                        : sf::Vector2f{m_x - prevX, newY - prevY};
}

void ParabolicObstacle::draw(sf::RenderWindow& window) const noexcept {
    window.draw(m_sprite);
}

sf::FloatRect ParabolicObstacle::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

// ─── Memento API ────────────────────────────────────────────────────────────
ParabolicObstacle::Snapshot ParabolicObstacle::saveState() const {
    return { m_x, m_movingLeft, m_speedMult, m_lastVel };
}

void ParabolicObstacle::restoreState(const Snapshot& snap, sf::Vector2u windowSize) {
    m_x          = snap.x;
    m_movingLeft = snap.movingLeft;
    m_speedMult  = snap.speedMult;
    m_lastVel    = snap.lastVel;
    m_sprite.setPosition(m_x, parabolicY(m_x, windowSize));
}

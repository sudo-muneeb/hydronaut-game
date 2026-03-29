#pragma once
#include "Obstacle.hpp"
#include <string>

// ─── SimulationEnvironment 2 — Sine-wave sea-urchin obstacle ──────────────────────────────────
// Moves left while tracing a sine wave scaled to window height.
class SineObstacle : public Obstacle {
public:
    SineObstacle(sf::Vector2u windowSize, const std::string& textureName);

    void          update(sf::Vector2u windowSize) noexcept override;
    void          draw(sf::RenderWindow& window)  const noexcept override;
    sf::FloatRect getBounds()                     const noexcept override;
    void          reset(sf::Vector2u windowSize)  override;
    const sf::Sprite& getSprite()                 const noexcept { return m_sprite; }

    // ─── Memento API ──────────────────────────────────────────────────────
    struct Snapshot {
        float x;
        float speedMult;
        sf::Vector2f lastVel;
    };
    Snapshot saveState() const;
    void     restoreState(const Snapshot& snap, sf::Vector2u windowSize);

private:
    float sineY(float x, sf::Vector2u win) const noexcept;

    sf::Sprite m_sprite;
    float      m_x;
};

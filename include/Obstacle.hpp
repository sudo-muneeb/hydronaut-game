#pragma once
#include <SFML/Graphics.hpp>
#include <algorithm>

// ─── Abstract Obstacle base ──────────────────────────────────────────────────
// All obstacle types implement update() and draw() with respect to
// the live window dimensions. Speed can be scaled by setSpeedMultiplier()
// (used by the Sonar Pulse to slow obstacles within its radius).
//
// For RL: each subclass tracks its frame-by-frame centre displacement in
// m_lastVel so the agent can observe both position AND velocity without
// needing an explicit direction flag.
class Obstacle {
public:
    virtual ~Obstacle() = default;

    virtual void update(sf::Vector2u windowSize) noexcept = 0;
    virtual void draw(sf::RenderWindow& window)  const noexcept = 0;
    virtual sf::FloatRect getBounds()            const noexcept = 0;
    virtual void reset(sf::Vector2u windowSize)  = 0;

    // ─── Sonar slow: sets a [0..1] multiplier applied to movement speed.
    void setSpeedMultiplier(float m) noexcept {
        m_speedMult = (m < 0.f) ? 0.f : (m > 1.f ? 1.f : m);
    }

    // ─── RL: frame velocity (pixel/frame displacement in each axis).
    // Updated by each subclass's update() as  Δcentre = newCentre - prevCentre.
    sf::Vector2f getVelocity() const noexcept { return m_lastVel; }

protected:
    float        m_speedMult = 1.0f;
    sf::Vector2f m_lastVel   {0.f, 0.f};  // set by subclass update()

    // ─── Window-relative sprite scaling ──────────────────────────────────────
    // Scales 'sprite' so it occupies (windowHeightFraction * windowSize.y) pixels
    // in height, preserving aspect ratio.  Call this from reset() AND from
    // update() so that live window resizes are picked up every frame.
    //
    // Default fraction 0.1f 
    // obstacle sprites.  Starfish uses 0.2f (see StarfishObstacle).
    static void applySpriteScale(sf::Sprite& sprite,
                                 sf::Vector2u windowSize,
                                 float windowHeightFraction = 0.1f) noexcept {
        if (windowSize.y == 0) return;
        int texH = sprite.getTextureRect().height;
        if (texH <= 0) return;
        float targetH = static_cast<float>(windowSize.y) * windowHeightFraction;
        float s       = targetH / static_cast<float>(texH);
        sprite.setScale(s, s);
    }
};


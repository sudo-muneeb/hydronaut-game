#pragma once
#include "Obstacle.hpp"
#include <SFML/Graphics.hpp>
#include <vector>
#include <limits>

// ─── Level 1 — Sprite-based Starfish obstacle ─────────────────────────────────
// Scrolls right-to-left, slowly spinning.  Sprite is re-scaled every frame via
// Obstacle::applySpriteScale() so it always occupies STARFISH_FRACTION of the
// shorter window edge — fully resolution-independent.
class StarfishObstacle : public Obstacle {
public:
    explicit StarfishObstacle(sf::Vector2u windowSize);

    void          update(sf::Vector2u windowSize) noexcept override;
    void          draw(sf::RenderWindow& window)  const noexcept override;
    sf::FloatRect getBounds()                     const noexcept override;
    void          reset(sf::Vector2u windowSize)  override;
    void          setSpeed(float speed)           noexcept;
    const sf::Sprite& getSprite()             const noexcept { return m_sprite; }

    // ─── Memento API ──────────────────────────────────────────────────────────
    struct Snapshot {
        float x, y;
        float speed;
        float rotation;
    };
    Snapshot saveState() const;
    void     restoreState(const Snapshot& snap, sf::Vector2u windowSize);

private:
    sf::Sprite m_sprite;
    float      m_x, m_y;
    float      m_speed;
    float      m_rotation;

};

// ─── ObstacleSnapshot — used by the RL state vector ───────────────────────────
struct ObstacleSnapshot {
    sf::Vector2f center;    // screen-space centre of the obstacle
    sf::Vector2f velocity;  // frame-by-frame displacement (px/frame)
};

// ─── Managed pool of StarfishObstacles for Level 1 ────────────────────────────
class StarfishObstaclePool {
public:
    void update(sf::Vector2u windowSize, float speed);
    void draw(sf::RenderWindow& window)               const;
    void drawDebugBounds(sf::RenderWindow& window)    const;
    bool collidesWithPlayer(const sf::Sprite& playerSprite,
                            const sf::Image&  playerImage) const noexcept;

    // RL: returns up to maxCount snapshots sorted by proximity to playerPos.
    // Pads with zero entries if fewer obstacles are active.
    std::vector<ObstacleSnapshot> getSnapshots(sf::Vector2f playerPos,
                                               std::size_t  maxCount) const noexcept;

    // ─── Memento API ──────────────────────────────────────────────────────────
    struct Snapshot {
        std::vector<StarfishObstacle::Snapshot> obstacles;
        float speed;
    };
    Snapshot saveState() const;
    void     restoreState(const Snapshot& snap, sf::Vector2u windowSize);

private:
    void spawnOne(sf::Vector2u windowSize);

    std::vector<StarfishObstacle> m_obstacles;
    float                         m_speed = 4.f;
};


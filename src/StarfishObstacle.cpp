// ─── StarfishObstacle.cpp ─────────────────────────────────────────────────────
// Level 1 obstacle: starfish sprite scrolling right-to-left with gentle spin.
//
// Scaling contract
// ────────────────
// applyWindowScale() is called on EVERY update() frame (not just reset).
// It delegates to Obstacle::applySpriteScale() which uses the live 'windowSize'
// argument — the game window size, NOT the device resolution.  This means
// resizing the OS window instantly resizes all active starfish proportionally.
// ─────────────────────────────────────────────────────────────────────────────
#include "StarfishObstacle.hpp"
#include "AssetManager.hpp"
#include <cstdlib>
#include <cmath>
#include <algorithm>
#include <stdexcept>
#include <limits>
#include <numeric>
#include "SpriteBounds.hpp"


const float SCALE_FACTOR = 0.15f;  // fraction of window height occupied by starfish sprites

// ─── StarfishObstacle ─────────────────────────────────────────────────────────

StarfishObstacle::StarfishObstacle(sf::Vector2u windowSize)
    : m_speed(4.f), m_rotation(0.f)
{
    m_sprite.setTexture(AssetManager::instance().texture("starfish"));

    // Centre the origin so rotation looks natural.
    auto tb = m_sprite.getLocalBounds();
    m_sprite.setOrigin(tb.width * 0.5f, tb.height * 0.5f);

    reset(windowSize);
}

void StarfishObstacle::reset(sf::Vector2u windowSize) {
    if (windowSize.x == 0 || windowSize.y == 0)
        throw std::invalid_argument("StarfishObstacle: window size must be non-zero");

    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR );

    m_x = static_cast<float>(windowSize.x);
    m_y = static_cast<float>(std::rand() % windowSize.y);
    m_sprite.setPosition(m_x, m_y);
    m_sprite.setRotation(m_rotation);
    m_lastVel = {0.f, 0.f};
}

void StarfishObstacle::setSpeed(float speed) noexcept { m_speed = speed; }

void StarfishObstacle::update(sf::Vector2u windowSize) noexcept {
    // Re-scale every frame so live window resizes propagate instantly.
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR );

    float step  = m_speed * m_speedMult;
    m_x        -= step;
    m_rotation += 1.2f * m_speedMult;   // gentle slow spin

    m_sprite.setPosition(m_x, m_y);
    m_sprite.setRotation(m_rotation);
    m_lastVel = {-step, 0.f};
}

void StarfishObstacle::draw(sf::RenderWindow& window) const noexcept {
    window.draw(m_sprite);
}

sf::FloatRect StarfishObstacle::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

// ─── StarfishObstaclePool ─────────────────────────────────────────────────────

void StarfishObstaclePool::update(sf::Vector2u windowSize, float speed) {
    m_speed = speed;

    if (m_obstacles.capacity() < 30)
        m_obstacles.reserve(30);

    if (std::rand() % 50 == 0)
        spawnOne(windowSize);

    for (auto& obs : m_obstacles)
        obs.update(windowSize);

    m_obstacles.erase(
        std::remove_if(m_obstacles.begin(), m_obstacles.end(),
            [](const StarfishObstacle& o) {
                auto b = o.getBounds();
                return b.left + b.width < 0.f;
            }),
        m_obstacles.end());
}

void StarfishObstaclePool::draw(sf::RenderWindow& window) const {
    for (const auto& obs : m_obstacles)
        obs.draw(window);
}

void StarfishObstaclePool::drawDebugBounds(sf::RenderWindow& window) const {
    auto& am = AssetManager::instance();
    for (const auto& obs : m_obstacles) {
        drawDebugHull(window, obs.getSprite(), am.hullUV("starfish"), sf::Color(255, 80, 80, 220));
    }
}

bool StarfishObstaclePool::collidesWithPlayer(const sf::Sprite& playerSprite,
                                              const sf::Image&  playerImage) const noexcept {
    auto& am = AssetManager::instance();
    for (const auto& obs : m_obstacles) {
        if (pixelPerfectOverlap(playerSprite, playerImage,
                                obs.getSprite(), am.image("starfish"))) {
            return true;
        }
    }
    return false;
}

std::vector<ObstacleSnapshot>
StarfishObstaclePool::getSnapshots(sf::Vector2f playerPos,
                                    std::size_t  maxCount) const noexcept {
    struct Candidate {
        float        distSq;
        sf::Vector2f center;
        sf::Vector2f velocity;
    };
    std::vector<Candidate> cands;
    cands.reserve(m_obstacles.size());

    for (const auto& obs : m_obstacles) {
        sf::FloatRect b = obs.getBounds();
        sf::Vector2f  c{ b.left + b.width * 0.5f, b.top + b.height * 0.5f };
        float dx = c.x - playerPos.x;
        float dy = c.y - playerPos.y;
        cands.push_back({ dx*dx + dy*dy, c, obs.getVelocity() });
    }

    std::sort(cands.begin(), cands.end(),
              [](const Candidate& a, const Candidate& b) {
                  return a.distSq < b.distSq;
              });

    std::vector<ObstacleSnapshot> result;
    result.reserve(maxCount);
    for (std::size_t i = 0; i < maxCount && i < cands.size(); ++i)
        result.push_back({ cands[i].center, cands[i].velocity });

    while (result.size() < maxCount)
        result.push_back({ {0.f, 0.f}, {0.f, 0.f} });

    return result;
}

void StarfishObstaclePool::spawnOne(sf::Vector2u windowSize) {
    StarfishObstacle obs(windowSize);
    obs.setSpeed(m_speed);
    m_obstacles.emplace_back(std::move(obs));
}

// ─── Memento ──────────────────────────────────────────────────────────────────

StarfishObstacle::Snapshot StarfishObstacle::saveState() const {
    return { m_x, m_y, m_speed, m_rotation };
}

void StarfishObstacle::restoreState(const Snapshot& snap, sf::Vector2u windowSize) {
    m_x        = snap.x;
    m_y        = snap.y;
    m_speed    = snap.speed;
    m_rotation = snap.rotation;
    applySpriteScale(m_sprite, windowSize, SCALE_FACTOR);
    m_sprite.setPosition(m_x, m_y);
    m_sprite.setRotation(m_rotation);
}

StarfishObstaclePool::Snapshot StarfishObstaclePool::saveState() const {
    Snapshot snap;
    snap.speed = m_speed;
    snap.obstacles.reserve(m_obstacles.size());
    for (const auto& obs : m_obstacles)
        snap.obstacles.push_back(obs.saveState());
    return snap;
}

void StarfishObstaclePool::restoreState(const Snapshot& snap, sf::Vector2u windowSize) {
    m_speed = snap.speed;
    m_obstacles.clear();
    m_obstacles.reserve(snap.obstacles.size());
    for (const auto& oSnap : snap.obstacles) {
        StarfishObstacle obs(windowSize);
        obs.restoreState(oSnap, windowSize);
        m_obstacles.push_back(std::move(obs));
    }
}

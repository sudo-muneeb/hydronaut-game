#pragma once
#include <SFML/Graphics.hpp>
#include <vector>

// ─── Single particle ──────────────────────────────────────────────────────────
struct Particle {
    enum class Kind {
        Bubble,
        ToxicSmoke
    };

    sf::Vector2f pos;
    sf::Vector2f vel;
    float        alphaf;     // 0.0–255.0
    float        life;       // seconds remaining
    float        radius;
    Kind         kind = Kind::Bubble;
};

// ─── Particle emitter owned by the Player ────────────────────────────────────
// Spawns bubble/wake particles at the submarine rear.
// Updated and drawn every frame via the SimulationEnvironment draw step.
class ParticleEmitter {
public:
    // Spawn N bubbles at 'origin' with a base drift velocity.
    void emit(sf::Vector2f origin, sf::Vector2f baseVel, int count = 1);

    // Spawn N dark, upward-drifting particles to represent toxic underwater smoke.
    void emitToxicSmoke(sf::Vector2f origin, sf::Vector2f sourceVel, int count = 1);

    // dt in seconds (1/60).
    void update(float dt) noexcept;

    void draw(sf::RenderWindow& window) const;

    bool empty() const noexcept { return m_particles.empty(); }
    void clear() noexcept { m_particles.clear(); }

private:
    std::vector<Particle> m_particles;
};

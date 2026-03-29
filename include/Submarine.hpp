#pragma once
// ─── Submarine.hpp ──────────────────────────────────────────────────────
// Context class for the State Pattern.
//
// Submarine (formerly Player) delegates ALL update-loop behaviour to
// its current IEntityState. It contains no conditional state logic itself —
// that logic lives exclusively in NormalState and DashState.
//
// This class provides the state objects with full access to its internals
// via protected mutators, keeping physics data private while enabling
// State objects to drive transitions through changeState().
// ─────────────────────────────────────────────────────────────────────────────
#include <SFML/Graphics.hpp>
#include <memory>
#include "Particle.hpp"
#include "EntityState.hpp"

class Submarine {
public:
    explicit Submarine(sf::Vector2u windowSize);
    ~Submarine() = default;

    // ─── Core interface (used by SimulationEnvironment, InputHandler, RL agent) ──────────
    // Delegates input to the active State — no behaviour conditionals here.
    // Returns true if a dash was fired this frame.
    bool applyCommand(int action, bool dashRequested) noexcept;

    // Delegates physics update to the active State.
    void update(float dt) noexcept;

    void draw(sf::RenderWindow& window);
    void reset(sf::Vector2u windowSize);

    // ─── Memento API ──────────────────────────────────────────────────────
    struct Snapshot {
        sf::Vector2f                  pos;
        sf::Vector2f                  vel;
        float                         dashClockSec;
        bool                          dashFired;
        bool                          dashActive;
        std::unique_ptr<IEntityState> stateObj;
    };
    Snapshot saveState() const;
    void     restoreState(const Snapshot& snap);

    // ─── Hitboxes ─────────────────────────────────────────────────────────
    sf::FloatRect getBounds()      const noexcept;
    sf::FloatRect getGrazeBounds() const noexcept;

    // ─── Getters used by RL / SimulationEnvironment ───────────────────────────────────────
    bool  isDashing()              const noexcept;
    bool  dashAvailable()          const noexcept;
    float dashCooldownRemaining()  const noexcept;
    sf::Vector2f getPosition()     const noexcept;
    sf::Vector2f getVelocity()     const noexcept { return m_vel; }
    const sf::Sprite& getSprite()  const noexcept { return m_sprite; }

    void setWindowSize(sf::Vector2u ws) noexcept { m_winSize = ws; }
    void applyImpulse(sf::Vector2f delta) noexcept { m_vel += delta; }

    // ─── Debug ────────────────────────────────────────────────────────────
    void drawDebugHitboxes(sf::RenderWindow& window) const;

    // ─── State machine transition (called by IEntityState subclasses) ─────
    void changeState(std::unique_ptr<IEntityState> newState) noexcept;

    // ─── State-accessible physics mutators ───────────────────────────────
    // Only IEntityState implementations should call these; they are public
    // so the state objects (non-friend classes) can drive the entity fully.
    void         setVelocity(sf::Vector2f v) noexcept   { m_vel = v; }
    void         addVelocity(sf::Vector2f dv) noexcept  { m_vel += dv; }
    void         integratePosition() noexcept;
    void         clampToWindow() noexcept;
    void         tickParticles(float dt) noexcept;
    void         emitDashParticles(float dt) noexcept;
    void         spawnDashTrail(int count);
    void         markDashFired() noexcept;
    bool         isDashAvailable() const noexcept;

private:
    sf::Sprite       m_sprite;
    sf::Vector2f     m_vel;
    sf::Clock        m_dashClock;
    bool             m_dashFired   = false;
    ParticleEmitter  m_particles;
    sf::Vector2u     m_winSize     = {};
    bool             m_dashActive  = false;   // true while in DashState

    std::unique_ptr<IEntityState> m_state;
};

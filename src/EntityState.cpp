#include "EntityState.hpp"
#include "Submarine.hpp"
#include "Constants.hpp"
#include <cmath>
#include <algorithm>

// ─── NormalState ─────────────────────────────────────────────────────────────
void NormalState::handleInput(Submarine& entity,
                              int              action,
                              bool             dashRequested) noexcept {
    // Directional acceleration — entity has no knowledge of which state is active.
    switch (action) {
        case 0: entity.addVelocity({0.f, -PLAYER_ACCEL}); break;  // Up
        case 1: entity.addVelocity({0.f,  PLAYER_ACCEL}); break;  // Down
        case 2: entity.addVelocity({-PLAYER_ACCEL, 0.f}); break;  // Left
        case 3: entity.addVelocity({ PLAYER_ACCEL, 0.f}); break;  // Right
        default: break;                                             // 4 = None
    }

    // Hydro-Dash: state governs the transition to DashState.
    if (dashRequested && entity.isDashAvailable()) {
        sf::Vector2f vel = entity.getVelocity();
        float len = std::sqrt(vel.x * vel.x + vel.y * vel.y);
        if (len < 0.5f) entity.setVelocity({PLAYER_MAX_SPEED, 0.f});

        sf::Vector2f boosted = entity.getVelocity();
        entity.setVelocity({ boosted.x * DASH_VEL_MULTIPLIER,
                              boosted.y * DASH_VEL_MULTIPLIER });
        entity.markDashFired();
        entity.spawnDashTrail(12);

        // State governs its own transition
        entity.changeState(std::make_unique<DashState>(DASH_IFRAME_FRAMES));
    }
}

void NormalState::update(Submarine& entity, float dt) noexcept {
    // Water drag
    sf::Vector2f vel = entity.getVelocity();
    vel.x *= PLAYER_DRAG;
    vel.y *= PLAYER_DRAG;

    // Speed cap
    float len = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    if (len > PLAYER_MAX_SPEED) {
        float inv = PLAYER_MAX_SPEED / len;
        vel.x *= inv;
        vel.y *= inv;
    }

    entity.setVelocity(vel);
    entity.integratePosition();
    entity.clampToWindow();
    entity.tickParticles(dt);
}

// ─── DashState ────────────────────────────────────────────────────────────────
void DashState::handleInput(Submarine& entity,
                             int              action,
                             bool             /*dashRequested*/) noexcept {
    // Reduced steering during dash — still steer but at ¼ strength
    switch (action) {
        case 0: entity.addVelocity({0.f, -PLAYER_ACCEL * 0.25f}); break;
        case 1: entity.addVelocity({0.f,  PLAYER_ACCEL * 0.25f}); break;
        case 2: entity.addVelocity({-PLAYER_ACCEL * 0.25f, 0.f}); break;
        case 3: entity.addVelocity({ PLAYER_ACCEL * 0.25f, 0.f}); break;
        default: break;
    }
    // Cannot initiate another dash while in DashState.
}

void DashState::update(Submarine& entity, float dt) noexcept {
    // Apply drag but no speed cap during dash
    sf::Vector2f vel = entity.getVelocity();
    vel.x *= PLAYER_DRAG;
    vel.y *= PLAYER_DRAG;
    entity.setVelocity(vel);

    entity.integratePosition();
    entity.clampToWindow();

    entity.emitDashParticles(dt);

    // State governs its own expiry transition
    --m_framesLeft;
    if (m_framesLeft <= 0) {
        entity.changeState(std::make_unique<NormalState>());
    }
}

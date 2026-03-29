#pragma once
// ─── EntityState.hpp ──────────────────────────────────────────────────────────
// State Pattern for Submarine.
//
// The entity's update loop behaviour is entirely delegated to its current
// IEntityState. State transitions are governed by the State objects themselves:
//   NormalState  ─── dash requested ────▶  DashState
//   DashState    ─── iframe countdown ─▶  NormalState
// ─────────────────────────────────────────────────────────────────────────────
#include <memory>

class Submarine;   // forward declaration

// ─── IEntityState ─────────────────────────────────────────────────────────────
// Uniform interface. Concrete states implement the full behavioural contract;
// the entity never selects behaviour via conditionals.
class IEntityState {
public:
    virtual ~IEntityState() = default;

    // Process directional action + ability input for this frame.
    // May trigger a state transition by calling entity.changeState().
    virtual void handleInput(Submarine& entity,
                             int              action,
                             bool             dashRequested) noexcept = 0;

    // Apply physics (drag, speed-cap, position integration, particles).
    // May trigger a state transition (e.g., iframe expiry).
    virtual void update(Submarine& entity, float dt) noexcept = 0;

    // ─── Memento Support ──────────────────────────────────────────────────
    virtual std::unique_ptr<IEntityState> clone() const = 0;
};

// ─── NormalState ──────────────────────────────────────────────────────────────
// Represents the submarine in standard cruise mode: water drag, speed cap,
// directional acceleration, and ability to enter a dash.
class NormalState final : public IEntityState {
public:
    void handleInput(Submarine& entity,
                     int              action,
                     bool             dashRequested) noexcept override;

    void update(Submarine& entity, float dt) noexcept override;

    std::unique_ptr<IEntityState> clone() const override {
        return std::make_unique<NormalState>(*this);
    }
};

// ─── DashState ────────────────────────────────────────────────────────────────
// The submarine is immune to collisions and moving at boosted speed.
// The state counts down iframe ticks and transitions back to NormalState.
class DashState final : public IEntityState {
public:
    explicit DashState(int iframes) noexcept : m_framesLeft(iframes) {}

    void handleInput(Submarine& entity,
                     int              action,
                     bool             dashRequested) noexcept override;

    void update(Submarine& entity, float dt) noexcept override;

    std::unique_ptr<IEntityState> clone() const override {
        return std::make_unique<DashState>(*this);
    }

private:
    int m_framesLeft;
};

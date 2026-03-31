#pragma once
// ─── RewardStrategy.hpp ───────────────────────────────────────────────────────
// Strategy Pattern for reward calculation.
//
// The IRewardStrategy absorbs all conditional reward logic that previously
// lived inside Level1/Level2/Level3::step() and ::update(). 
//
// Concrete strategies are injected into the SimulationEnvironment (environment manager) via
// setRewardStrategy(), allowing the environment's physics loop to be
// completely agnostic to how rewards are shaped.
// ─────────────────────────────────────────────────────────────────────────────

// ─── EnvironmentContext ───────────────────────────────────────────────────────
// A plain aggregate of everything a RewardStrategy needs. Produced by the
// SimulationEnvironment and passed into the strategy each frame — zero coupling to entity types.
struct EnvironmentContext {
    float playerSpeed     = 0.f;
    bool  treasureCollected = false;
    bool  lethalCollision   = false;
    float distToTreasure    = -1.f;    // < 0 means no tracking yet
    float prevDistToTreasure = -1.f;
    int   idleFrames        = 0;
    bool  trainingMode      = false;
    int   grazeCount        = 0;       // graze coincidences this frame
    bool  dashUsed          = false;
    bool  sonarUsed         = false;
};

// ─── RewardResult ─────────────────────────────────────────────────────────────
// Returned from calculateReward so that the SimulationEnvironment can apply it without
// re-implementing any logic.
struct RewardResult {
    float reward      = 0.f;
    bool  episodeDone = false;
};

// ─── IRewardStrategy ──────────────────────────────────────────────────────────
class IRewardStrategy {
public:
    virtual ~IRewardStrategy() = default;

    // Calculates the reward for the current step given only the context.
    // Framework (SimulationEnvironment) must not contain any reward conditionals after injection.
    virtual RewardResult calculateReward(const EnvironmentContext& ctx) = 0;
};

// ─── Level1RewardStrategy ─────────────────────────────────────────────────────
// Pure survival: +0.15/step, -1 on death, -1 after 100 idle frames.
class Level1RewardStrategy final : public IRewardStrategy {
public:
    RewardResult calculateReward(const EnvironmentContext& ctx) override;
};

// ─── Level2RewardStrategy ─────────────────────────────────────────────────────
// Treasure approach shaping + +100 on collection + -1 on death + idleness.
class Level2RewardStrategy final : public IRewardStrategy {
public:
    RewardResult calculateReward(const EnvironmentContext& ctx) override;
};

// ─── Level3RewardStrategy ─────────────────────────────────────────────────────
// Same as Level2 but with different enemy types — strategy encapsulates
// any per-level reward tweaks cleanly.
class Level3RewardStrategy final : public IRewardStrategy {
public:
    RewardResult calculateReward(const EnvironmentContext& ctx) override;
};

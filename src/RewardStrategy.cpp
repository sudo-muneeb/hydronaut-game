#include "RewardStrategy.hpp"
#include <algorithm>
#include <cmath>

// ─── Level1RewardStrategy ─────────────────────────────────────────────────────
// Pure survival: flat +0.15/step, idle penalty, -100 on death.
RewardResult Level1RewardStrategy::calculateReward(const EnvironmentContext& ctx) {
    RewardResult result;

    result.reward = 0.15f;

    if (ctx.idleFrames > 100) {
        result.reward -= 100.f;
        if (ctx.trainingMode) result.episodeDone = true;
    }

    if (ctx.lethalCollision) {
        result.reward  -= 100.f;
        result.episodeDone = true;
    }

    return result;
}

// ─── Level2RewardStrategy ─────────────────────────────────────────────────────
// Treasure approach shaping + +100 pick-up + -100 death + idle penalty.
RewardResult Level2RewardStrategy::calculateReward(const EnvironmentContext& ctx) {
    RewardResult result;

    result.reward = 0.15f;

    // Approach shaping: reward moving toward the treasure
    if (ctx.prevDistToTreasure > 0.f && ctx.distToTreasure >= 0.f) {
        float approach = (ctx.prevDistToTreasure - ctx.distToTreasure) * 0.05f;
        result.reward += approach;
    }

    // Idleness penalty
    if (ctx.idleFrames > 100) {
        result.reward -= 100.f;
        if (ctx.trainingMode) result.episodeDone = true;
    }

    // Treasure collected
    if (ctx.treasureCollected) {
        result.reward += 100.f;
    }

    // Lethal collision
    if (ctx.lethalCollision) {
        result.reward  -= 100.f;
        result.episodeDone = true;
    }

    return result;
}

// ─── Level3RewardStrategy ─────────────────────────────────────────────────────
// Same structure as Level2. Encapsulating separately allows per-level tuning
// without touching any other reward class.
RewardResult Level3RewardStrategy::calculateReward(const EnvironmentContext& ctx) {
    RewardResult result;

    result.reward = 0.15f;

    if (ctx.prevDistToTreasure > 0.f && ctx.distToTreasure >= 0.f) {
        float approach = (ctx.prevDistToTreasure - ctx.distToTreasure) * 0.05f;
        result.reward += approach;
    }

    if (ctx.idleFrames > 100) {
        result.reward -= 100.f;
        if (ctx.trainingMode) result.episodeDone = true;
    }

    if (ctx.treasureCollected) {
        result.reward += 100.f;
    }

    if (ctx.lethalCollision) {
        result.reward  -= 100.f;
        result.episodeDone = true;
    }

    return result;
}

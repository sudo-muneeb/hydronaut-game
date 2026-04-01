#include "RewardStrategy.hpp"
#include <algorithm>
#include <cmath>

// ─── Level1RewardStrategy ─────────────────────────────────────────────────────
// Pure survival: tiny +0.05/step, no idle penalty (environment self-corrects).
// Death penalty dominates survival ticks — agent must avoid obstacles.
RewardResult Level1RewardStrategy::calculateReward(const EnvironmentContext& ctx) {
    RewardResult result;
    result.reward = 0.05f;  // Minimal survival tick

    if (ctx.lethalCollision) {
        result.reward      = -50.f;  // Severe penalty — must dominate ~1000 survival ticks
        result.episodeDone = true;
        return result;               // Early return — nothing else matters
    }

    // No idle penalty: obstacles come to player. Hiding is valid IF it works
    // (but eventually starfish will reach the corner and end the episode naturally).

    return result;
}

// ─── Level2RewardStrategy ─────────────────────────────────────────────────────
// Treasure hunt: approach shaping (0.3x reward multiplier) + prize/penalty parity.
// Death and treasure are equally weighted: -100 death vs +100 treasure.
RewardResult Level2RewardStrategy::calculateReward(const EnvironmentContext& ctx) {
    RewardResult result;

    // ── Lethal collision — terminal, overrides everything ──────────────────────
    if (ctx.lethalCollision) {
        result.reward      = -100.f;  // Matches treasure +100 — parity enforcement
        result.episodeDone = true;
        return result;               // Early return — no accumulation
    }

    // ── Idle timeout — terminal if training, override everything ─────────────
    if (ctx.idleFrames > 200 && ctx.trainingMode) {
        result.reward      = -50.f;   // Significant penalty, but less than death
        result.episodeDone = true;
        return result;
    }

    // ── Normal step — only reaches here if alive and not idle-timeout ─────────
    // Approach shaping: reward moving toward treasure (increased from 0.05 to 0.3)
    if (ctx.prevDistToTreasure > 0.f && ctx.distToTreasure >= 0.f) {
        float delta = ctx.prevDistToTreasure - ctx.distToTreasure;
        result.reward += delta * 0.3f;  // Highly rewarding progress
    }

    // Treasure collected
    if (ctx.treasureCollected) {
        result.reward += 100.f;
    }

    return result;
}

// ─── Level3RewardStrategy ─────────────────────────────────────────────────────
// Same structure as Level2. Encapsulating separately allows per-level tuning
// without touching any other reward class.
RewardResult Level3RewardStrategy::calculateReward(const EnvironmentContext& ctx) {
    RewardResult result;

    // ── Lethal collision — terminal, overrides everything ──────────────────────
    if (ctx.lethalCollision) {
        result.reward      = -100.f;  // Matches treasure +100 — parity enforcement
        result.episodeDone = true;
        return result;               // Early return — no accumulation
    }

    // ── Idle timeout — terminal if training, override everything ─────────────
    if (ctx.idleFrames > 200 && ctx.trainingMode) {
        result.reward      = -50.f;   // Significant penalty, but less than death
        result.episodeDone = true;
        return result;
    }

    // ── Normal step — only reaches here if alive and not idle-timeout ─────────
    // Approach shaping: reward moving toward treasure (increased from 0.05 to 0.3)
    if (ctx.prevDistToTreasure > 0.f && ctx.distToTreasure >= 0.f) {
        float delta = ctx.prevDistToTreasure - ctx.distToTreasure;
        result.reward += delta * 0.3f;  // Highly rewarding progress
    }

    // Treasure collected
    if (ctx.treasureCollected) {
        result.reward += 100.f;
    }

    return result;
}

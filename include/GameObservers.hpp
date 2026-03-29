#pragma once
// ─── GameObservers.hpp ────────────────────────────────────────────────────────
// Concrete observer implementations for the Hydronaut event system.
//
// Each observer subscribes to specific event types via the SimulationEnvironment's EventBus
// and reacts uniformly through the IObserver::notify() interface.
//
// Observers operate via reference injection only — they never own the state
// they write to. This maintains consistent GoF decoupling: the observing
// system has zero knowledge of the entity that published the event.
// ─────────────────────────────────────────────────────────────────────────────
#include "EventBus.hpp"
#include <any>
#include <iostream>

// ─── ScoreObserver ────────────────────────────────────────────────────────────
// Listens for "ScoreAdded" and "GrazeScored" events and increments the score.
class ScoreObserver final : public IObserver {
public:
    explicit ScoreObserver(int& score, int& grazeAcc)
        : m_score(score), m_grazeAcc(grazeAcc) {}

    void notify(const Event& event) override {
        if (event.type == "ScoreAdded") {
            int amount = std::any_cast<int>(event.payload);
            m_score    += amount;
        } else if (event.type == "GrazeScored") {
            int amount  = std::any_cast<int>(event.payload);
            m_score    += amount;
            m_grazeAcc += amount;
        }
    }

private:
    int& m_score;
    int& m_grazeAcc;
};

// ─── RewardObserver ───────────────────────────────────────────────────────────
// Listens for "RewardGranted" and "RewardPenalty" events, accumulates them
// into the current frame reward, and tracks the step-since-reward counter.
class RewardObserver final : public IObserver {
public:
    RewardObserver(float& reward, int& stepsSinceReward, float& lastReward)
        : m_reward(reward)
        , m_stepsSinceReward(stepsSinceReward)
        , m_lastReward(lastReward) {}

    void notify(const Event& event) override {
        if (event.type == "RewardGranted") {
            m_reward += std::any_cast<float>(event.payload);
            m_stepsSinceReward = 0;
        } else if (event.type == "RewardPenalty") {
            m_reward -= std::any_cast<float>(event.payload);
            m_stepsSinceReward = 0;
        }
    }

private:
    float& m_reward;
    int&   m_stepsSinceReward;
    float& m_lastReward;
};

// ─── ShakeObserver ────────────────────────────────────────────────────────────
// Listens for "ScreenShake" and forwards to a callback (the SimulationEnvironment's
// triggerShake).  Decouples emitters from knowledge of SimulationEnvironment internals.
class ShakeObserver final : public IObserver {
    using ShakeFn = std::function<void(int, float)>;
public:
    explicit ShakeObserver(ShakeFn shakeCallback)
        : m_callback(std::move(shakeCallback)) {}

    void notify(const Event& event) override {
        if (event.type == "ScreenShake") {
            auto p = std::any_cast<ShakeParams>(event.payload);
            m_callback(p.frames, p.intensity);
        }
    }

private:
    ShakeFn m_callback;
};

// ─── TelemetryObserver ────────────────────────────────────────────────────────
// Listens for any event and prints a structured telemetry line to stdout.
// Useful as a catch-all diagnostic observer during development.
class TelemetryObserver final : public IObserver {
public:
    void notify(const Event& event) override {
        std::cout << "[TELEMETRY] event=" << event.type << "\n";
    }
};

#pragma once
// ─── EventBus.hpp ─────────────────────────────────────────────────────────────
// Observer Pattern event bus infrastructure.
//
// Entities publish events rather than calling external systems directly.
// Observer systems subscribe to specific event types and react uniformly
// via the notify(event) interface.
// ─────────────────────────────────────────────────────────────────────────────
#include <string>
#include <any>
#include <vector>
#include <unordered_map>
#include <algorithm>

// ─── Event ────────────────────────────────────────────────────────────────────
// A fully type-erased event: a string type tag and an arbitrary payload.
//
// Defined event type strings:
//   "PlayerCollision"   payload: none
//   "TreasureCollected" payload: none
//   "ScoreAdded"        payload: int  (amount)
//   "RewardGranted"     payload: float (delta)
//   "RewardPenalty"     payload: float (magnitude, > 0)
//   "GrazeScored"       payload: int  (graze point count)
//   "ScreenShake"       payload: ShakeParams
struct Event {
    std::string type;
    std::any    payload;   // empty (monostate) when unused
};

// ─── ShakeParams ──────────────────────────────────────────────────────────────
// Payload type for "ScreenShake" events.
struct ShakeParams {
    int   frames    = 0;
    float intensity = 0.f;
};

// ─── IObserver ────────────────────────────────────────────────────────────────
// Uniform reaction interface across all decoupled observing systems.
class IObserver {
public:
    virtual ~IObserver() = default;
    virtual void notify(const Event& event) = 0;
};

// ─── EventBus ─────────────────────────────────────────────────────────────────
// Central pub-sub subject. Owned by SimulationEnvironment and shared to all systems
// that need to post or receive game events.
class EventBus {
public:
    // Subscribe an observer to a specific event type.
    void subscribe(const std::string& eventType, IObserver* observer);

    // Unsubscribe an observer from a specific event type.
    void unsubscribe(const std::string& eventType, IObserver* observer);

    // Publish an event to all subscribers registered for event.type.
    void publish(const Event& event);

private:
    std::unordered_map<std::string, std::vector<IObserver*>> m_subscribers;
};

#pragma once
#include <memory>

class SimulationEnvironment;
class Level1;
class Level2;
class Level3;

// Opaque base structure for polymorphic state storage
struct InternalStateSnapshot {
    virtual ~InternalStateSnapshot() = default;
};

// ─── SimulationMemento ────────────────────────────────────────────────────────
// Securely holds the raw data state of a SimulationEnvironment.
// The external training loop can hold this object, but cannot mutate its internal data.
class SimulationMemento final {
public:
    explicit SimulationMemento(std::unique_ptr<InternalStateSnapshot> state)
        : m_state(std::move(state)) {}
    ~SimulationMemento() = default;

    // Fast episodic resets require moving, not deep-copying, mementos around
    SimulationMemento(SimulationMemento&&) noexcept = default;
    SimulationMemento& operator=(SimulationMemento&&) noexcept = default;

private:
    // Strictly prevent arbitrary external mutation
    friend class SimulationEnvironment;
    friend class Level1;
    friend class Level2;
    friend class Level3;

    std::unique_ptr<InternalStateSnapshot> m_state;
};

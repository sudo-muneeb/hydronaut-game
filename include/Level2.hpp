#pragma once
#include "SimulationEnvironment.hpp"
#include "Submarine.hpp"
#include "SineObstacle.hpp"
#include "ParabolicObstacle.hpp"
#include "Treasure.hpp"
#include <vector>

// ─── SimulationEnvironment 2 — Arc of Chaos ───────────────────────────────────────────────────
// Sine-wave urchin + parabolic crab. Collect treasure chests to score.
class Level2 : public SimulationEnvironment {
public:
    explicit Level2(sf::RenderWindow& window);

    // ─── RL environment API ────────────────────────────────────────────────
    // 12-element normalised state (see implementation_plan for layout).
    std::vector<float> getState()                                   const override;
    std::vector<float> reset(sf::Vector2u windowSize)                     override;
    std::vector<float> step(int action, float& reward, bool& isDone)      override;

    const Submarine& getPlayer() const override { return m_player; }
    Submarine&       getPlayer()       override { return m_player; }

    std::unique_ptr<SimulationMemento> create_memento() const override;
    void restore_memento(const SimulationMemento& memento) override;

protected:
    bool update() override;
    void draw()   override;

private:
    Submarine    m_player;
    SineObstacle       m_sine;
    ParabolicObstacle  m_para;
    Treasure           m_treasure;
    float              m_prevDistToTreasure = -1.f;  // for distance shaping
};

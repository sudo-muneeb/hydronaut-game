#pragma once
#include <SFML/Window/Keyboard.hpp>
#include "CommandQueue.hpp"
#include <memory>

// ─── InputHandler ─────────────────────────────────────────────────────────────
// Translates live keyboard state into discrete Command objects for telemetry
// and decoupled execution, as well as preserving the legacy 0-14 composite
// action format for the RL agent's replay buffer.
struct InputHandler {
    // Legacy mapping for RL HumanTrainer
    static int pollCompositeAction() noexcept {
        int base = 4; // None
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::Up))    base = 0;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Down))  base = 1;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Left))  base = 2;
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Right)) base = 3;

        int mod = 0;
        if (sf::Keyboard::isKeyPressed(sf::Keyboard::X))         mod = 2; // Sonar
        else if (sf::Keyboard::isKeyPressed(sf::Keyboard::Space)) mod = 1; // Dash

        return base + (mod * 5);
    }
    
    static void decodeAction(int composite, int& baseDir, bool& isDash, bool& isSonar) noexcept {
        baseDir = composite % 5;
        int mod = composite / 5;
        isDash  = (mod == 1);
        isSonar = (mod == 2);
    }

    // Command Factory & Telemetry Queueing
    static void pollAndQueue(CommandQueue& queue) {
        int baseDir; bool isDash; bool isSonar;
        int action = pollCompositeAction();
        decodeAction(action, baseDir, isDash, isSonar);

        // Core movement command (or idle)
        if (baseDir < 4) {
            queue.push(std::make_unique<MoveCommand>(baseDir));
        } else {
            queue.push(std::make_unique<IdleCommand>());
        }

        // Abilities (dash and sonar prioritize sonar in the integer logic)
        if (isSonar) {
            queue.push(std::make_unique<SonarCommand>());
        } else if (isDash) {
            queue.push(std::make_unique<DashCommand>());
        }
    }
};

#include <iostream>
#include <memory>
#include <SFML/Graphics.hpp>
#include "Level1.hpp"
#include "SimulationMemento.hpp"
#include "AssetManager.hpp"
#include "Submarine.hpp"

int main() {
    std::cout << "[Mock] Init...\n";
    sf::RenderWindow window(sf::VideoMode(800, 600), "Mock", sf::Style::None);
    window.setVisible(false);

    try {
        AssetManager::instance().loadAll();
    } catch(...) {
        std::cout << "Asset loading failed (maybe run from wrong dir?)\n";
    }

    Level1 env(window);
    env.reset({800, 600});

    std::cout << "--- Taking 10 initial steps ---\n";
    for (int i=0; i<10; ++i) {
        float r; bool d;
        env.step(4, r, d); // action 4 = idle
    }
    
    sf::Vector2f pos10 = env.getPlayer().getPosition();
    std::cout << "Step 10 Pos: (" << pos10.x << ", " << pos10.y << ")\n";

    std::cout << "--- Creating Memento ---\n";
    auto memento = env.create_memento();

    std::cout << "--- Taking 10 divergent steps ---\n";
    for (int i=0; i<10; ++i) {
        float r; bool d;
        env.step(1, r, d); // action 1 = move down
    }

    sf::Vector2f pos20 = env.getPlayer().getPosition();
    std::cout << "Step 20 Pos: (" << pos20.x << ", " << pos20.y << ")\n";

    std::cout << "--- Restoring Memento ---\n";
    env.restore_memento(*memento);

    sf::Vector2f posRestored = env.getPlayer().getPosition();
    std::cout << "Restored Pos: (" << posRestored.x << ", " << posRestored.y << ")\n";

    if (pos10 == posRestored) {
        std::cout << "SUCCESS: exact state match!\n";
    } else {
        std::cout << "FAILED: mismatch.\n";
    }
    return 0;
}

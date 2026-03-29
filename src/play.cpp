// ─── play.cpp ────────────────────────────────────────────────────────────────
// Inference visualiser — loads hydronaut_dqn.pt and drives the submarine using
// env->step() for physics, then env->renderFrame() for the real game visuals.
// Action timing: 1–4 Hz (15–60 frame hold), matching training cadence.
//
// Usage:  ./play_hydronaut [1|2|3]   (default: level 2)

#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <random>

#include "Constants.hpp"
#include "AssetManager.hpp"
#include "Level1.hpp"
#include "Level2.hpp"
#include "Level3.hpp"
#include "DQNAgent.hpp"
#include "RLConfig.hpp"

static constexpr int HOLD_MIN = 15;
static constexpr int HOLD_MAX = 60;

int main(int argc, char* argv[]) {
    int levelChoice = 2;
    if (argc >= 2) {
        try { levelChoice = std::stoi(argv[1]); }
        catch (...) {}
    }
    if (levelChoice < 1 || levelChoice > 3) {
        std::cerr << "Usage: play_hydronaut [1|2|3]\n";
        return EXIT_FAILURE;
    }

    try {
        // ── Window at full desktop resolution ─────────────────────────────────
        // The window always tracks the actual screen — no hardcoded dimensions.
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

        sf::RenderWindow window(
            desktop,
            "Hydronaut — AI Play  (SimulationEnvironment " + std::to_string(levelChoice) + ")",
            sf::Style::Default);
        window.setFramerateLimit(60);

        AssetManager::instance().loadAll();

        sf::Music music;
        if (music.openFromFile(ASSET_MUSIC)) {
            music.setLoop(true); music.setVolume(10.f); music.play();
        }

        // ── Build level ──────────────────────────────────────────────────────
        std::unique_ptr<SimulationEnvironment> env;
        switch (levelChoice) {
            case 1:  env = std::make_unique<Level1>(window); break;
            case 2:  env = std::make_unique<Level2>(window); break;
            default: env = std::make_unique<Level3>(window); break;
        }
        env->setTrainingMode(false);

        // ── Load trained model ───────────────────────────────────────────────
        DQNAgent agent;
        agent.load_model(RLConfig::MODEL_PATH);
        agent.setEpsilon(0.0f);

        std::cout << "[play] SimulationEnvironment " << levelChoice
                  << " | State dim: " << RLConfig::STATE_DIM
                  << " | Action hold: " << HOLD_MIN << "-" << HOLD_MAX << " frames\n"
                  << "       ESC or close window to quit.\n\n";

        // RNG for hold duration
        std::mt19937 rng(std::random_device{}());
        std::uniform_int_distribution<int> holdDist(HOLD_MIN, HOLD_MAX);

        auto& font = AssetManager::instance().font();
        int episodeCount = 0;

        while (window.isOpen()) {
            ++episodeCount;
            // Always read the current window size per episode — adapts to any resize.
            const sf::Vector2u winSize = window.getSize();
            env->setVirtualSize(winSize);
            std::vector<float> state = env->reset(winSize);

            float totalReward   = 0.f;
            int   frameCount    = 0;
            int   decisionCount = 0;
            int   holdRemaining = 0;
            int   currentAction = 0;

            while (window.isOpen()) {
                // ── Events ────────────────────────────────────────────────────
                sf::Event event;
                while (window.pollEvent(event)) {
                    if (event.type == sf::Event::Closed ||
                        (event.type == sf::Event::KeyPressed &&
                         event.key.code == sf::Keyboard::Escape)) {
                        window.close();
                    }
                    if (event.type == sf::Event::Resized) {
                        // Keep view in sync with actual window size — no clamping
                        unsigned w = event.size.width;
                        unsigned h = event.size.height;
                        env->setVirtualSize({w, h});
                    }
                }
                if (!window.isOpen()) break;

                // ── Human-paced decision ────────────────────────────────────
                if (holdRemaining == 0) {
                    currentAction = agent.choose_action(state);
                    holdRemaining = holdDist(rng);
                    ++decisionCount;
                }
                --holdRemaining;

                // ── Step environment ────────────────────────────────────────
                float reward = 0.f;
                bool  done   = false;
                state = env->step(currentAction, reward, done);
                totalReward += reward;
                ++frameCount;

                env->renderFrame();

                if (done) {
                    std::cout << "Episode " << episodeCount
                              << " | Frames: " << frameCount
                              << " | Decisions: " << decisionCount
                              << " | Reward: " << totalReward << "\n";
                    sf::sleep(sf::milliseconds(600));
                    break;
                }
            }
        }

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

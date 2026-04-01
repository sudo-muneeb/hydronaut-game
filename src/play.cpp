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
#include "InputHandler.hpp"

static constexpr int HOLD_MIN = 15;
static constexpr int HOLD_MAX = 60;

// ─── Helper: convert action to key display string ────────────────────────────
std::string actionToKeyDisplay(int action) {
    int baseDir;
    bool dashReq, sonarReq;
    InputHandler::decodeAction(action, baseDir, dashReq, sonarReq);

    std::string keyStr;
    
    // Direction
    switch (baseDir) {
        case 0: keyStr = "↑"; break; // Up
        case 1: keyStr = "↓"; break; // Down
        case 2: keyStr = "←"; break; // Left
        case 3: keyStr = "→"; break; // Right
        default: keyStr = "◯"; break; // Idle
    }
    
    // Special ability
    if (sonarReq) keyStr += " + X";
    else if (dashReq) keyStr += " + SPACE";
    
    return keyStr;
}

int main(int argc, char* argv[]) {
    int levelChoice = 2;
    unsigned seed = RLConfig::RANDOM_SEED;  // Fixed seed for reproducible playback
    
    if (argc >= 2) {
        try { levelChoice = std::stoi(argv[1]); }
        catch (...) {}
    }
    if (argc >= 3) {
        try { seed = std::stoul(argv[2]); }
        catch (...) {}
    }
    
    if (levelChoice < 1 || levelChoice > 3) {
        std::cerr << "Usage: play_hydronaut [1|2|3] [seed]\n";
        return EXIT_FAILURE;
    }

    try {
        // ── Fixed 1920x1080 window for consistent AI training/evaluation ──────
        sf::VideoMode playMode(1920u, 1080u);

        sf::RenderWindow window(
            playMode,
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
        agent.setSeed(seed);  // Set seed for reproducible playback

        std::cout << "[play] SimulationEnvironment " << levelChoice
                  << " | State dim: " << RLConfig::STATE_DIM
                  << " | Seed: " << seed
                  << " | Action hold: " << HOLD_MIN << "-" << HOLD_MAX << " frames\n"
                  << "       Resolution: 1920x1080 (fixed)\n"
                  << "       ESC or close window to quit.\n\n";

        // RNG for hold duration — use fixed seed for reproducibility
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> holdDist(HOLD_MIN, HOLD_MAX);

        auto& font = AssetManager::instance().font();
        int episodeCount = 0;
        sf::Vector2u fixedPlaySize(1920u, 1080u);

        while (window.isOpen()) {
            ++episodeCount;
            // Always use fixed 1920x1080 for consistent AI evaluation
            env->setVirtualSize(fixedPlaySize);
            std::vector<float> state = env->reset(fixedPlaySize);

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
                    // Fixed size: ignore resize events
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

                // ── Render game with AI action display ──────────────────────
                std::string keyDisplay = actionToKeyDisplay(currentAction);
                env->renderFrameWithOverlay(keyDisplay, 
                    sf::Color(30, 60, 100, 210),           // Dark blue background
                    sf::Color(200, 200, 200, 180));        // Transparent white text

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

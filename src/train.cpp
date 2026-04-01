#include <SFML/Graphics.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>
#include <string>
#include <array>
#include <iomanip>
#include <random>

#include "Constants.hpp"
#include "AssetManager.hpp"
#include "Level1.hpp"
#include "Level2.hpp"
#include "Level3.hpp"
#include "ReplayBuffer.hpp"
#include "DQNAgent.hpp"
#include "RLConfig.hpp"

int main(int argc, char* argv[]) {
    int episodes = RLConfig::EPISODES;
    unsigned seed = RLConfig::RANDOM_SEED;  // Default seed for reproducibility
    
    if (argc >= 2) {
        try { episodes = std::stoi(argv[1]); }
        catch (...) { std::cerr << "inv ep count using " << RLConfig::EPISODES << "\n"; }
    }
    if (argc >= 3) {
        try { seed = std::stoul(argv[2]); }
        catch (...) { std::cerr << "inv seed using " << RLConfig::RANDOM_SEED << "\n"; }
    }

    try {
        // hidden window to keep math correct
        sf::RenderWindow window(
            sf::VideoMode(1920, 1080),
            "training", sf::Style::None);
        window.setVisible(false);

        AssetManager::instance().loadAll();
        
        // init levels
        Level1 lvl1(window); lvl1.setTrainingMode(true);
        Level2 lvl2(window); lvl2.setTrainingMode(true);
        Level3 lvl3(window); lvl3.setTrainingMode(true);

        std::array<SimulationEnvironment*, 3> levels{ &lvl1, &lvl2, &lvl3 };

        ReplayBuffer buffer(RLConfig::BUFFER_CAPACITY);
        DQNAgent     agent;
        
        // Set seeds for reproducibility
        buffer.setSeed(seed);
        agent.setSeed(seed);

        sf::Vector2u trainSize(1920u, 1080u);
        lvl1.reset(trainSize);
        lvl2.reset(trainSize);
        lvl3.reset(trainSize);

        // rng setup — use fixed seed for reproducibility
        std::mt19937 rng(seed);
        std::uniform_int_distribution<int> holdDist(RLConfig::HOLD_MIN, RLConfig::HOLD_MAX);

        std::cout << "[train] Episodes: " << episodes
                  << " | Seed: " << seed
                  << " | Buffer: " << RLConfig::BUFFER_CAPACITY
                  << " | Batch: " << RLConfig::BATCH_SIZE << "\n\n";

        int globalFrame = 0;
        int current_phase = 1; 

        for (int ep = 0; ep < episodes; ++ep) {
            
            // curriculum phases
            if (ep == RLConfig::PHASE1_END_EP) {
                current_phase = 2;
            } else if (ep == RLConfig::PHASE2_END_EP) {
                current_phase = 3;
            }

            SimulationEnvironment* env = levels[current_phase - 1];

            // drain sfml events
            { sf::Event e; while (window.pollEvent(e)) {} }

            // Use fixed training size 1920x1080 for consistent policy learning
            env->setVirtualSize(trainSize);

            std::vector<float> state = env->reset(trainSize);

            float episodeReward   = 0.f;
            int   decisionCount   = 0;
            int   holdRemaining   = 0;
            int   currentAction   = 0;
            float accReward       = 0.f;
            std::vector<float> decisionState;

            for (int frame = 0; frame < RLConfig::MAX_STEPS; ++frame) {

                // new action
                if (holdRemaining == 0) {
                    decisionState  = state;
                    currentAction  = agent.choose_action(state);
                    holdRemaining  = holdDist(rng);
                    accReward      = 0.f;
                    ++decisionCount;
                }

                // run physics
                float stepReward = 0.f;
                bool  done       = false;
                std::vector<float> nextState = env->step(currentAction, stepReward, done);

                accReward     += stepReward;
                episodeReward += stepReward;
                --holdRemaining;
                ++globalFrame;

                // save exp
                bool holdEnded = (holdRemaining == 0);
                if (holdEnded || done) {
                    buffer.push({ decisionState, currentAction,
                                  accReward, nextState, done });
                    decisionState = nextState;   
                    accReward     = 0.f;
                    holdRemaining = 0;           
                }

                state = nextState;

                // backprop
                if (buffer.canSample(RLConfig::WARMUP_EXP) &&
                    (globalFrame % RLConfig::LEARN_EVERY == 0)) {
                    auto batch = buffer.sample(RLConfig::BATCH_SIZE);
                    agent.learn(batch);
                }

                if (done) break;
            }
            
            // decay eps at end of ep
            agent.decay_epsilon();

            if ((ep + 1) % RLConfig::PRINT_EVERY == 0 || ep == 0) {
                std::cout
                    << "ep " << std::setw(4) << (ep + 1)
                    << " | l"   << current_phase
                    << " | dec " << std::setw(4) << decisionCount
                    << " | rew " << std::setw(8) << std::fixed
                    << std::setprecision(1) << episodeReward
                    << " | eps " << std::setprecision(4) << agent.getEpsilon()
                    << " | buf " << buffer.size()
                    << "\n";
            }
        }

        agent.save_model(RLConfig::MODEL_PATH);

    } catch (const std::exception& e) {
        std::cerr << "err " << e.what() << "\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

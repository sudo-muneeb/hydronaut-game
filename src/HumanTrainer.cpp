#include "HumanTrainer.hpp"
#include "DQNAgent.hpp"
#include <iostream>
#include <filesystem>

static const char* MODEL_PATH = "hydronaut_dqn.pt";
static const int   LEARN_EVERY = 4;
static const int   SAVE_EVERY  = 1000; // frames
static const int   WARMUP_EXP  = 64;

HumanTrainer& HumanTrainer::instance() {
    static HumanTrainer instance;
    return instance;
}

HumanTrainer::HumanTrainer() = default;
HumanTrainer::~HumanTrainer() {
    // Intentionally empty. Do not save here, as LibTorch might be tearing down.
    // Call shutdown() explicitly at the end of main().
}

void HumanTrainer::shutdown() {
    if (m_initialized) {
        saveModel();
        // Explicitly release torch modules before global destruction
        m_agent.reset();
        m_buffer.reset();
        m_initialized = false;
    }
}

void HumanTrainer::initIfNeeded() {
    if (m_initialized) return;

    m_agent  = std::make_unique<DQNAgent>();
    m_buffer = std::make_unique<ReplayBuffer>(50000);

    // Try to load user's existing trained weights
    if (std::filesystem::exists(MODEL_PATH)) {
        try {
            m_agent->load_model(MODEL_PATH);
            std::cout << "[HumanTrainer] Loaded existing model: " << MODEL_PATH << "\n";
        } catch (const std::exception& e) {
            std::cerr << "[HumanTrainer] Warning: Failed to load model (likely dimension mismatch). Starting fresh.\n";
        }
    } else {
        std::cout << "[HumanTrainer] No existing model found. Will train from scratch.\n";
    }

    // Defensive: ensure the online network is always in training mode before
    // any learn() calls, regardless of what load_model() may have done internally.
    // (The root cause of the model corruption bug was load_model calling eval().)
    m_agent->ensureTrainMode();

    // Low epsilon since we're mostly learning off-policy from a human,
    // but the DQN structure still needs it for internal Q-updates.
    m_agent->setEpsilon(0.01f);
    m_initialized = true;
}

void HumanTrainer::recordExperience(
    const std::vector<float>& state,
    int action,
    float reward,
    const std::vector<float>& next_state,
    bool done
) {
    if (action < 0 || action > 14) return; // Ignore invalid actions

    initIfNeeded();

    m_buffer->push({state, action, reward, next_state, done});
    m_frameCount++;

    if (m_buffer->size() > WARMUP_EXP && (m_frameCount % LEARN_EVERY == 0)) {
        m_agent->learn(m_buffer->sample(WARMUP_EXP));
    }

    if (done || (m_frameCount % SAVE_EVERY == 0)) {
        saveModel();
    }
}

void HumanTrainer::saveModel() {
    if (m_initialized && m_agent) {
        m_agent->save_model(MODEL_PATH);
        std::cout << "[HumanTrainer] Saved model (" << m_frameCount << " frames trained).\n";
    }
}

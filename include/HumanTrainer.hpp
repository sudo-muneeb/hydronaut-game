#pragma once
#include <vector>
#include <memory>

class DQNAgent;
class ReplayBuffer;

class HumanTrainer {
public:
    static HumanTrainer& instance();

    // Feed a human gameplay experience into the RL replay buffer.
    // Automatically schedules training steps and saves the model periodically.
    void recordExperience(
        const std::vector<float>& state,
        int action,
        float reward,
        const std::vector<float>& next_state,
        bool done
    );

    // Force save the current network weights
    void saveModel();

    // Explicitly save the model and cleanup LibTorch state before exit
    void shutdown();

private:
    HumanTrainer();
    ~HumanTrainer();

    // Lazy load the network only if the player actually enables the feature
    void initIfNeeded();

    std::unique_ptr<DQNAgent>     m_agent;
    std::unique_ptr<ReplayBuffer> m_buffer;

    bool m_initialized = false;
    int  m_frameCount  = 0;
};

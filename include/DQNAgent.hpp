#pragma once
// ─── DQNAgent.hpp ────────────────────────────────────────────────────────────
// Header-only Deep Q-Network agent using LibTorch (C++17, cxx11 ABI).
//
// Network architecture:
//   Linear(50 → 128) → ReLU → Linear(128 → 128) → ReLU → Linear(128 → 15)
//
// Training:
//   ε-greedy exploration with exponential decay.
//   Bellman target = r + γ * max(Q_target(s')) for non-terminal transitions.
//   Soft target-network update:  θ_target ← τ·θ_online + (1-τ)·θ_target

#include <torch/torch.h>
#include "ReplayBuffer.hpp"
#include "RLConfig.hpp"
#include <string>
#include <vector>
#include <random>
#include <algorithm>
#include <iostream>

// ─── Neural Network ──────────────────────────────────────────────────────────
struct DQNNetImpl : torch::nn::Module {
    torch::nn::Linear fc1{nullptr}, fc2{nullptr}, fc3{nullptr};

    DQNNetImpl(int inputDim, int hiddenDim, int outputDim) : torch::nn::Module("DQNNet") {
        fc1 = register_module("fc1", torch::nn::Linear(inputDim,  hiddenDim));
        fc2 = register_module("fc2", torch::nn::Linear(hiddenDim, hiddenDim));
        fc3 = register_module("fc3", torch::nn::Linear(hiddenDim, outputDim));
    }

    torch::Tensor forward(torch::Tensor x) {
        x = torch::relu(fc1->forward(x));
        x = torch::relu(fc2->forward(x));
        return fc3->forward(x);          // raw Q-values (no activation)
    }
};
TORCH_MODULE(DQNNet);   // creates shared_ptr wrapper "DQNNet"

// ─── DQN Agent ───────────────────────────────────────────────────────────────
class DQNAgent {
public:
    DQNAgent()
        : m_onlineNet(RLConfig::STATE_DIM, RLConfig::HIDDEN_DIM, RLConfig::ACTION_DIM)
        , m_targetNet(RLConfig::STATE_DIM, RLConfig::HIDDEN_DIM, RLConfig::ACTION_DIM)
        , m_optimizer(m_onlineNet->parameters(),
                      torch::optim::AdamOptions(RLConfig::LR))
        , m_epsilon(RLConfig::EPS_START)
        , m_rng(std::random_device{}())
        , m_dis01(0.f, 1.f)
        , m_disAction(0, RLConfig::ACTION_DIM - 1)
    {
        // Initialise target net with same weights as online net
        hardUpdateTarget();
        m_onlineNet->train();
        m_targetNet->eval();
    }

    // ── Set epsilon directly (e.g. 0.0 for pure exploitation) ────────────────
    void setEpsilon(float eps) noexcept { m_epsilon = eps; }
    float getEpsilon() const noexcept   { return m_epsilon; }

    // ── Ensure online net is in training mode ─────────────────────────────────
    // Call this after load_model() as a defensive guard — the online net must
    // always be in train() mode when learn() is invoked.
    void ensureTrainMode() { m_onlineNet->train(); }

    // ── ε-greedy action selection ─────────────────────────────────────────────
    int choose_action(const std::vector<float>& state) {
        if (m_dis01(m_rng) < m_epsilon)
            return m_disAction(m_rng);   // explore

        torch::NoGradGuard no_grad;
        auto t = torch::from_blob(
                     const_cast<float*>(state.data()),
                     {1, RLConfig::STATE_DIM},
                     torch::kFloat32).clone();
        auto q_vals = m_onlineNet->forward(t);   // [1, RLConfig::ACTION_DIM]
        return static_cast<int>(q_vals.argmax(1).item<int64_t>());
    }

    // ── Learning step (Bellman equation + MSE loss) ───────────────────────────
    void learn(const std::vector<Experience>& batch) {
        if (batch.empty()) return;

        const int N = static_cast<int>(batch.size());

        // Build tensors from batch
        std::vector<float> states_v, next_states_v, rewards_v;
        std::vector<int64_t> actions_v;
        std::vector<float> dones_v;

        states_v.reserve(N * RLConfig::STATE_DIM);
        next_states_v.reserve(N * RLConfig::STATE_DIM);
        rewards_v.reserve(N);
        actions_v.reserve(N);
        dones_v.reserve(N);

        for (const auto& e : batch) {
            states_v.insert(states_v.end(), e.state.begin(), e.state.end());
            next_states_v.insert(next_states_v.end(),
                                 e.next_state.begin(), e.next_state.end());
            rewards_v.push_back(e.reward);
            actions_v.push_back(static_cast<int64_t>(e.action));
            dones_v.push_back(e.done ? 1.f : 0.f);
        }

        auto states      = torch::from_blob(states_v.data(),
                                {N, RLConfig::STATE_DIM}, torch::kFloat32).clone();
        auto next_states = torch::from_blob(next_states_v.data(),
                                {N, RLConfig::STATE_DIM}, torch::kFloat32).clone();
        auto rewards     = torch::from_blob(rewards_v.data(),
                                {N, 1},         torch::kFloat32).clone();
        auto actions     = torch::from_blob(actions_v.data(),
                                {N, 1},         torch::kInt64).clone();
        auto dones       = torch::from_blob(dones_v.data(),
                                {N, 1},         torch::kFloat32).clone();

        // Current Q-values for chosen actions: Q(s,a)
        auto current_q = m_onlineNet->forward(states).gather(1, actions);

        // Target Q-values: r + γ * max_a' Q_target(s',a') * (1 - done)
        torch::Tensor target_q;
        {
            torch::NoGradGuard no_grad;
            auto next_q = std::get<0>(m_targetNet->forward(next_states).max(1, true));
            target_q    = rewards + RLConfig::GAMMA * next_q * (1.f - dones);
        }

        // MSE loss
        auto loss = torch::mse_loss(current_q, target_q);

        m_optimizer.zero_grad();
        loss.backward();
        // Gradient clipping for stability
        torch::nn::utils::clip_grad_norm_(m_onlineNet->parameters(), 10.0);
        m_optimizer.step();

        // Soft target network update
        softUpdateTarget();
    }

    // decay eps per ep instead of per step
    void decay_epsilon() {
        m_epsilon = std::max(RLConfig::EPS_END, m_epsilon * RLConfig::EPS_DECAY);
    }


    // ── Model persistence ─────────────────────────────────────────────────────
    void save_model(const std::string& path) {
        std::string tmp_path = path + ".tmp";
        torch::save(m_onlineNet, tmp_path);
        if (std::rename(tmp_path.c_str(), path.c_str()) != 0) {
            std::cerr << "[DQNAgent] Failed to atomic-rename model to " << path << "\n";
        } else {
            std::cout << "[DQNAgent] Model saved to: " << path << "\n";
        }
    }

    void load_model(const std::string& path) {
        torch::load(m_onlineNet, path);
        hardUpdateTarget();    // sync target net after loading
        m_onlineNet->train();  // MUST stay in train mode — eval() here is what caused weight corruption
        std::cout << "[DQNAgent] Model loaded from: " << path << "\n";
    }

private:
    void hardUpdateTarget() {
        auto online_params = m_onlineNet->parameters();
        auto target_params = m_targetNet->parameters();
        torch::NoGradGuard no_grad;
        for (std::size_t i = 0; i < online_params.size(); ++i)
            target_params[i].copy_(online_params[i]);
    }

    void softUpdateTarget() {
        torch::NoGradGuard no_grad;
        auto online_params = m_onlineNet->parameters();
        auto target_params = m_targetNet->parameters();
        for (std::size_t i = 0; i < online_params.size(); ++i)
            target_params[i].copy_(RLConfig::TAU * online_params[i] +
                                   (1.f - RLConfig::TAU) * target_params[i]);
    }

    DQNNet                         m_onlineNet;
    DQNNet                         m_targetNet;
    torch::optim::Adam             m_optimizer;
    float                          m_epsilon;
    std::mt19937                   m_rng;
    std::uniform_real_distribution<float> m_dis01;
    std::uniform_int_distribution<int>    m_disAction;
};

#pragma once
// ─── ReplayBuffer.hpp ─────────────────────────────────────────────────────────
// Header-only experience replay buffer for DQN training.
// Stores (state, action, reward, next_state, done) tuples in a circular deque.
#include <deque>
#include <vector>
#include <random>
#include <stdexcept>
#include <algorithm>
#include "RLConfig.hpp"

struct Experience {
    std::vector<float> state;
    int                action     = 0;
    float              reward     = 0.f;
    std::vector<float> next_state;
    bool               done       = false;
};

class ReplayBuffer {
public:
    explicit ReplayBuffer(std::size_t maxSize = 50'000)
        : m_maxSize(maxSize)
        , m_rng(RLConfig::RANDOM_SEED)  // Use fixed seed from RLConfig for reproducibility
    {}

    // ── Set RNG seed for reproducibility ───────────────────────────────────────
    void setSeed(unsigned seed) { m_rng.seed(seed); }

    // ── Push a new experience; evict the oldest if over capacity ─────────────
    void push(Experience exp) {
        if (m_buf.size() >= m_maxSize)
            m_buf.pop_front();
        m_buf.emplace_back(std::move(exp));
    }

    // ── Sample a random mini-batch (without replacement) ─────────────────────
    std::vector<Experience> sample(std::size_t batchSize) {
        if (m_buf.size() < batchSize)
            throw std::runtime_error("ReplayBuffer::sample — not enough experiences");

        // Build a shuffled index vector and take the first batchSize entries.
        std::vector<std::size_t> indices(m_buf.size());
        std::iota(indices.begin(), indices.end(), 0);
        std::shuffle(indices.begin(), indices.end(), m_rng);

        std::vector<Experience> batch;
        batch.reserve(batchSize);
        for (std::size_t i = 0; i < batchSize; ++i)
            batch.push_back(m_buf[indices[i]]);
        return batch;
    }

    std::size_t size() const noexcept { return m_buf.size(); }
    bool        canSample(std::size_t batchSize) const noexcept {
        return m_buf.size() >= batchSize;
    }

private:
    std::deque<Experience> m_buf;
    std::size_t            m_maxSize;
    std::mt19937           m_rng;
};

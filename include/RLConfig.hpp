#pragma once
// ─── RLConfig.hpp ─────────────────────────────────────────────────────────────
// Centralized configuration for all Reinforcement Learning hyperparameters.
// Single source of truth for DQN training, network architecture, and RL state
// representation. Modify these constants to experiment with different configs.
// ─────────────────────────────────────────────────────────────────────────────

#include <string>

namespace RLConfig {

// ─── Network Architecture ─────────────────────────────────────────────────────
// Define the neural network structure for the DQN agent.
inline constexpr int   STATE_DIM    = 50;   // Input: 50-dimensional state vector
inline constexpr int   HIDDEN_DIM   = 128;  // Hidden layers dimension
inline constexpr int   ACTION_DIM   = 15;   // Output: 5 directions × 3 abilities

// ─── Training Hyperparameters ─────────────────────────────────────────────────
// Core learning parameters for the DQN algorithm.
inline constexpr float GAMMA        = 0.99f;    // Discount factor for future rewards
inline constexpr float LR           = 1e-3f;    // Adam optimizer learning rate
inline constexpr float TAU          = 0.005f;   // Soft target network update factor
inline constexpr float EPS_START    = 1.0f;     // Initial exploration rate
inline constexpr float EPS_END      = 0.08f;    // Minimum exploration rate
inline constexpr float EPS_DECAY    = 0.995f;  // Epsilon decay per episode
inline constexpr int   BATCH_SIZE   = 64;       // Mini-batch size for training

// ─── Episode & Step Controls ──────────────────────────────────────────────────
// Training loop configuration.
inline constexpr int   EPISODES      = 2000;   // Total training episodes
inline constexpr int   MAX_STEPS     = 8000;   // Max steps per episode
inline constexpr int   LEARN_EVERY   = 4;      // Learn every N frames
inline constexpr int   WARMUP_EXP    = 64;     // Minimum experiences before learning
inline constexpr int   HOLD_MIN      = 4;      // Min frames to hold an action
inline constexpr int   HOLD_MAX      = 8;      // Max frames to hold an action
inline constexpr int   PRINT_EVERY   = 10;     // Print stats every N episodes

// ─── Replay Buffer ────────────────────────────────────────────────────────────
inline constexpr int   BUFFER_CAPACITY = 50'000;  // Experience replay buffer size

// ─── State Representation ─────────────────────────────────────────────────────
// Constants for encoding the game state into the neural network input vector.
inline constexpr int   MAX_OBJ_SLOTS  = 8;      // Max objects tracked in state
inline constexpr int   NUM_OBJ_TYPES  = 6;      // Type IDs 1-6 (0 = empty)
inline constexpr float MAX_VEL        = 10.f;   // Max velocity for normalization
inline constexpr float REF_W          = 1920.f; // Reference screen width
inline constexpr float REF_H          = 1080.f; // Reference screen height

// ─── Curriculum Learning ──────────────────────────────────────────────────────
// Episode thresholds for switching between levels during training.
inline constexpr int   PHASE1_END_EP  = 600;   // Switch to Level 2 after this ep
inline constexpr int   PHASE2_END_EP  = 1200;  // Switch to Level 3 after this ep

// ─── File Paths ───────────────────────────────────────────────────────────────
inline const char* MODEL_PATH = "hydronaut_dqn.pt";  // Trained model save location

} // namespace RLConfig

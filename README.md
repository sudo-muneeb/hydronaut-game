# Hydronaut 🚢

A 2D submarine dodge-and-collect game built with **SFML 2.6.1** and **C++17**, featuring a custom **Deep Q-Network (DQN) AI** trained via **LibTorch** to autonomously play the game!

Navigate your submarine through increasingly chaotic obstacle patterns across 3 distinct levels, or watch as the integrated neural network learns to dodge, survive, and hunt for treasure on its own.

---

## 🌟 Overview & Key Features

Hydronaut serves as both an entertaining survival/collection arcade game and a fully realized environment for Reinforcement Learning (RL). 

*   **Responsive 2D Engine**: Built robustly using C++17 and SFML, utilizing screen-relative mathematics allowing the game and the AI to scale seamlessly across any window resolution seamlessly.
*   **Three Distinct Challenges**:
    *   **Level 1 (Obstacles Unleashed):** Pure survival against aggressively swarming, rotating convex shapes.
    *   **Level 2 (Arc of Chaos):** Treasure hunting while dodging Sine-wave and Parabolic moving creatures.
    *   **Level 3 (Waves of Danger):** Advanced treasure hunting amidst enemies that stalk via complex Secant and Exponential-Sine mathematical curves.
*   **Integrated DQN Agent**: A robust Deep Reinforcement Learning agent completely capable of executing the game autonomously, built top-to-bottom using PyTorch's C++ library (LibTorch).
*   **Asset Management Singleton**: Ensures reliable memory and resource management keeping SFML textures and fonts protected from dangling pointers and reallocation mid-frame.

---

## 🧠 Deep Reinforcement Learning (AI)

At the heart of the project is a deep neural network that learned to conquer the game through thousands of simulated episodes without raw pixel data.

### Architecture
- **LibTorch C++ API**: Used to train the AI directly inside the C++ environment natively.
- **Centralized Configuration**: All RL hyperparameters consolidated in `include/RLConfig.hpp` for easy experimentation.
- **Model**: A multi-layer perceptron (MLP) mapping a 50-dimensional state space to 15 action outputs (5 directions × 3 ability states). The architecture uses 2 hidden layers (50 → 128 → 128 → 15).
- **Target Network & Experience Replay**: The agent stores 50,000 transition frames in a replay buffer, sampling random batches of 64 frames to stabilize Q-learning using delayed target soft-updates (Polyak averaging with τ=0.005).
- **Curriculum Learning**: Progressive training across levels (1→2→3) with phase transitions at episodes 600 and 1200.

### State & Reward Shaping
- **Normalized Observations**: The AI receives a 50-dimensional normalized (`[0, 1]` and `[-1, 1]`) float array carrying game context: Level ID, viewport dimensions, player velocity, timeout counters, proximity coordinates for up to 8 objects, and ability readiness signals.
- **Expanded Action Space**: The AI can now use special abilities (Dash for invulnerability, Sonar for slowdown) in addition to directional movement, learning WHEN to activate them strategically.
- **Goal-Oriented Rewards**: Level 1 rewards survival. Levels 2 & 3 incentivize approaching treasure, punish idle camping, and provide massive reward spikes (+200.0) for collecting treasure while treating enemy collision as lethal (-1.0).
- **Resolution Agnostic Training**: The AI trains across dynamically shifting resolutions `(640x480 to 1920x1080)` to teach spatial adaptation rather than memorizing routes.

### Configuration & Tuning
All hyperparameters are configurable in **`include/RLConfig.hpp`**:
- Network architecture (dimensions, layers)
- Training parameters (learning rate, epsilon decay, batch size)
- Episode controls (total episodes, max steps, curriculum phases)
- State representation (object slots, velocity normalization)

To experiment: Edit `RLConfig.hpp`, rebuild with CMake, and retrain. See `AI_README.md` for detailed tuning recommendations.

---

## 🚀 Building & Running

### Dependencies
- `libsfml-graphics`, `libsfml-window`, `libsfml-system`, `libsfml-audio`
- `g++` (Compiler with `-std=c++17` support)
- `LibTorch` / PyTorch C++ API (for AI components)
- `CMake` (for orchestrating the build)

### Quick Start

The included shell script will handle standard compilation and launch the game for human play:

```bash
bash run.sh
```

### Running the AI Executables
If the project is built via CMake, specific RL targets are produced:
*   **`./build/train_hydronaut [episodes]`**: Executes the game in rapid headless mode (no visual SFML window overhead), allowing the network to train policies across 2000 episodes (default) using curriculum learning. Saves the trained model to `hydronaut_dqn.pt`.
*   **`./build/play_hydronaut [1|2|3]`**: Opens the physical rendering window and runs the game in pure exploitation mode (ε=0.0) using the trained model. Watch the AI play Level 1, 2, or 3.

### AI Configuration & Experimentation
To modify RL hyperparameters:
```bash
# 1. Edit configuration
nano include/RLConfig.hpp

# 2. Rebuild
cd build && cmake --build . -j$(nproc)

# 3. Train with new settings
./build/train_hydronaut

# 4. Watch AI play
./build/play_hydronaut 2
```

See `AI_README.md` for detailed architecture documentation and tuning recommendations.

---

## 🎮 Controls (Human Mode)

| Key | Action |
|---|---|
| `↑ ↓ ← →` | Move the submarine |
| `Space` | Activate Hydro-Dash (invulnerability + speed boost) |
| `Shift` | Activate Sonar Pulse (slow down all enemies) |
| `P` | Pause / Resume |
| `Escape` | Return to main menu from any level / quit |
| `Enter` | Select menu option |

---

## 📂 Project Structure

```text
hydronaut/
├── assets/                  Media files (fonts, music, sprites)
├── include/                 Header files defining game objects & RL states
│   └── RLConfig.hpp         Centralized RL hyperparameter configuration
├── src/                     Source files (engine loop, physics, AI execution)
├── libtorch/                C++ PyTorch libraries used for DQN
├── CMakeLists.txt           Build definitions for Game, Train, and Play targets
├── AI_README.md             Detailed RL architecture & tuning documentation
├── run.sh                   Quick-start build/run script
└── hydronaut                Compiled main binary
```

**Memory & Safety Notes:**
All objects are strongly protected via double-exception catching per loop, window size safety clamps, strictly bound vectors replacing dynamic arrays per-frame, and deterministic memory teardown using `std::unique_ptr` per level execution.

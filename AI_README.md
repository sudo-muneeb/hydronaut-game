# Hydronaut: Reinforcement Learning (DQN) AI Implementation

This document details the architecture and implementation of the Deep Q-Network (DQN) agent that learns to play Hydronaut. The AI is built using **LibTorch** (PyTorch's C++ API) and interacts with the game engine without modifying the core simulation loop.

---

## 🏗️ Architecture Overview

The AI implementation is composed of several key components:

### 1. Centralized Configuration (`RLConfig.hpp`)
**NEW**: All RL hyperparameters are now consolidated in a single header file for easy experimentation.
- **Location**: `include/RLConfig.hpp`
- **Purpose**: Single source of truth for all training parameters, network architecture, and state representation constants
- **Benefits**: Change learning rates, epsilon decay, buffer sizes, or any parameter in ONE place
- **Organization**: Parameters grouped into logical sections:
  - Network Architecture (STATE_DIM=50, HIDDEN_DIM=128, ACTION_DIM=15)
  - Training Hyperparameters (GAMMA, LR, TAU, epsilon values, BATCH_SIZE)
  - Episode Controls (EPISODES, MAX_STEPS, LEARN_EVERY, etc.)
  - Replay Buffer (BUFFER_CAPACITY=50,000)
  - State Representation (MAX_OBJ_SLOTS, NUM_OBJ_TYPES, reference screen dimensions)
  - Curriculum Learning (phase transition thresholds)
  - File Paths (model save location)

### 2. The Environment API (`Level::step()`)
To train the AI without running the full visual game loop, the `Level` classes expose an RL-specific API:
- **`std::vector<float> reset(sf::Vector2u windowSize)`**: Resets the level state, randomizes positions, and returns the initial state vector.
- **`std::vector<float> step(int action, float& reward, bool& isDone)`**: Applies exactly one action, advances the physics simulation by one frame (`dt = 1/60s`), and returns the new state, the scalar reward, and a boolean indicating if the episode has ended (e.g. death).
- **`std::vector<float> getState() const`**: Computes the 50-dimensional normalized state vector representing the current frame.

### 3. The Agent (`DQNAgent.hpp`)
The `DQNAgent` handles the neural network model, action selection (using $\epsilon$-greedy exploration), and the Q-learning update step.
- **Model**: A multi-layer perceptron (MLP) with 2 hidden layers (`50 -> 128 -> 128 -> 15`).
- **Architecture**: Defined in `RLConfig::STATE_DIM`, `RLConfig::HIDDEN_DIM`, `RLConfig::ACTION_DIM`
- **Target Network**: A delayed copy of the main network used to stabilize the Q-learning targets (Polyak averaging with `TAU=0.005`).
- **Optimizer**: Adam optimizer with learning rate `LR=1e-3` (configurable in RLConfig.hpp).
- **Exploration**: ε-greedy policy with exponential decay from `EPS_START=1.0` to `EPS_END=0.08` at rate `EPS_DECAY=0.9999`.

### 4. The Replay Buffer (`ReplayBuffer.hpp`)
Experience Replay is used to break correlation between consecutive frames and allow the agent to learn from past experiences.
- **Capacity**: Stores the last `RLConfig::BUFFER_CAPACITY` (50,000) transitions `(state, action, reward, next_state, done)`.
- **Sampling**: Uniformly samples batches of size `RLConfig::BATCH_SIZE` (64) for training.

### 5. Advanced RL Engine Architecture (GoF Patterns)
To support robust telemetry gathering, imitation learning, and rapid RL episodic resets, the simulation engine is strictly decoupled using pure Gang of Four (GoF) design patterns:
- **Dependency Injection**: The core `GameApp` and all `SimulationEnvironment` game logic are isolated from concrete SFML rendering dependencies or the RL `HumanTrainer` singleton via pure abstract interfaces (`IInterfaces.hpp` and `SFMLAdapters.hpp`).
- **Command Pattern**: All physical inputs are decoupled from the simulation. The `InputHandler` generates discrete `Command` objects pushed to a `CommandQueue`, allowing flawless serialization to `telemetry.log` for imitation learning. 
- **Observer Pattern**: A Publish-Subscribe `EventBus` manages communications between core physics, telemetry loggers, and reward systems. `RewardObserver`, `ScoreObserver`, and `TrainerObserver` react to standard events uniformly (`RewardGranted`, `PlayerCollision`, `ExperienceGenerated`) without hardcoded linkages.
- **State & Strategy Patterns**: The `Submarine` flattens complex branching logic by delegating behavior directly to discrete State objects (`NormalState`, `DashState`). Meanwhile, `SimulationEnvironment` accepts injected `IRewardStrategy` implementations (`Level1RewardStrategy`, etc.) to dynamically swap score calculations.
- **Memento Pattern**: To avoid the heavy computational penalty of destroying and recreating the physics object graph during RL episodes, `SimulationEnvironment` acts as an Originator, exposing `create_memento()` and `restore_memento()` to rapidly snapshot and rollback the precise mathematical game state isolated inside an opaque `SimulationMemento` container.

---

## 🧠 State Representation (50 Dimensions)

The neural network receives a `50-dimensional` floating-point vector (defined as `RLConfig::STATE_DIM`), strictly normalized down to `[0, 1]` or `[-1, 1]` to aid neural network convergence.

| Index(es) | Description | Normalization |
| :--- | :--- | :--- |
| `0` | Level ID (0.0 for L1, 0.5 for L2, 1.0 for L3) | `[0, 1]` |
| `1, 2` | Current screen width / height (compared to 1920x1080) | `[0, 1]` |
| `3, 4` | Player X / Y coordinates | `[0, 1]` |
| `5, 6` | Player Velocity X / Y | `[-1, 1]` |
| `7` | Steps since last reward (timeout counter) | `[0, 1]` |
| `8` | Value of the last reward received | `[-1, 1]` |
| `9...46` | Object Slots (up to 8 slots, 5 floats per slot) | mixed |
| `47` | Dash ability available (1.0 = ready, 0.0 = cooldown) | `[0, 1]` |
| `48` | Sonar ability available (1.0 = ready, 0.0 = cooldown) | `[0, 1]` |
| `49` | Idleness penalty factor (increases when not moving) | `[0, 1]` |

**Object Slot Layout (5 floats per object, configured via `RLConfig::MAX_OBJ_SLOTS`):**
- `[0]` Object Type ID (scaled by `RLConfig::NUM_OBJ_TYPES`)
- `[1, 2]` Object X / Y coordinates `[0, 1]` (normalized to screen dimensions)
- `[3, 4]` Object Velocity X / Y `[-1, 1]` (normalized by `RLConfig::MAX_VEL`)

---

## 🎮 Action Space

The agent outputs a discrete action integer from `0` to `14` (defined as `RLConfig::ACTION_DIM = 15`), representing 5 directional movements × 3 ability states:

**Directional Actions (0-4):**
- `0`: Up
- `1`: Down
- `2`: Left
- `3`: Right
- `4`: None (Idle)

**Extended Action Space (5-14):**
- `5-9`: Directional movement + Dash ability
- `10-14`: Directional movement + Sonar ability

The expanded action space allows the AI to learn WHEN to use special abilities (Dash for invulnerability, Sonar for slowdown) in addition to basic navigation.

---

## 🏆 Reward Shaping

The reward functions vary slightly by level to guide the agent toward the exact behaviors we want to see.

### Level 1 (Survival)
The goal is simply to survive the swarm of triangles for as long as possible.
- **+1.0**: For every frame survived.
- **-1.0**: Upon collision with a triangle (Lethal).

### Level 2 & 3 (Treasure Hunting)
The agent must actively navigate the screen to catch the treasure chest while avoiding complex boss enemies. To prevent the agent from finding safe "blind spots" and camping indefinitely, the reward is heavily shaped:
- **+0.15**: Base survival reward per frame.
- **+Approach (distance_closed * 0.50)**: Strong positive reinforcement for moving *closer* to the treasure.
- **-0.50 (Idle Penalty)**: If the agent's speed drops below `0.5f`, it is punished to prevent camping.
- **+200.0**: Massive spike reward upon successfully intersecting the treasure chest.
- **-1.0**: Upon collision with an enemy (Lethal).

---

## 🚀 Training & Inference Pipelines

### Training (`src/train.cpp`)
- **Headless Execution**: Runs invisibly without rendering actual pixels.
- **Randomized Resolution**: The virtual screen size randomizes between `640x480` and `1920x1080` every episode so the agent learns relative space instead of memorizing fixed pixel routes.
- **Curriculum Learning**: Progressive training approach controlled by `RLConfig::PHASE1_END_EP` and `RLConfig::PHASE2_END_EP`:
  - Episodes 1-600: Level 1 only (survival training)
  - Episodes 601-1200: Level 2 (treasure hunting with moderate enemies)
  - Episodes 1201-2000: Level 3 (advanced treasure hunting with complex patterns)
- **Action Hold**: The fast simulation speed (`1/60s`) makes frame-by-frame action selection too chaotic. The agent selects an action and "holds" it for `RLConfig::HOLD_MIN` to `RLConfig::HOLD_MAX` frames (4-8 frames), mimicking human reaction times.
- **Learning Schedule**: Updates every `RLConfig::LEARN_EVERY` (4) frames after `RLConfig::WARMUP_EXP` (64) initial experiences.
- **Total Episodes**: Configurable via `RLConfig::EPISODES` (default: 2000).
- **Episode Length**: Maximum of `RLConfig::MAX_STEPS` (8000) per episode.

### Playback (`src/play.cpp`)
- Loads the trained model from `RLConfig::MODEL_PATH` (default: `hydronaut_dqn.pt`).
- Hooks into the SFML window, physically rendering every frame.
- Uses `epsilon = 0.0` (pure exploitation) to navigate the level using the learned weights.
- Action hold duration: 15-60 frames for smoother human-viewable gameplay.

---

## ⚙️ Configuration & Experimentation

### Quick Parameter Tuning

All hyperparameters are centralized in **`include/RLConfig.hpp`**. To experiment with different configurations:

1. **Edit the configuration file:**
   ```bash
   nano include/RLConfig.hpp
   ```

2. **Rebuild the project:**
   ```bash
   cd build && cmake --build . -j$(nproc)
   ```

3. **Train with new settings:**
   ```bash
   ./build/train_hydronaut
   ```

4. **Test the trained model:**
   ```bash
   ./build/play_hydronaut 2   # Level 2 (or 1, 3)
   ```

### Recommended Experiments

**For Faster Convergence:**
- Increase `LR` to `2e-3` or `5e-3`
- Decrease `EPS_DECAY` to `0.999` for faster exploration reduction
- Increase `LEARN_EVERY` to reduce gradient update frequency

**For Better Performance:**
- Increase `HIDDEN_DIM` to `256` for more model capacity
- Increase `BUFFER_CAPACITY` to `100'000` for richer experience diversity
- Extend `MAX_STEPS` to `10'000` for longer episodes

**For Curriculum Tuning:**
- Adjust `PHASE1_END_EP` to spend more/less time on basic survival
- Modify `PHASE2_END_EP` to control intermediate difficulty training duration

**For State Space Experiments:**
- Change `MAX_OBJ_SLOTS` to track more/fewer obstacles
- Adjust `MAX_VEL` to modify velocity normalization range

### Parameter Reference

See `include/RLConfig.hpp` for the complete list of configurable parameters with inline documentation. All constants are marked `inline constexpr` for zero runtime overhead.

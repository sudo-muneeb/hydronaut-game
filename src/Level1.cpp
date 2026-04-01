#include "Level1.hpp"
#include "Constants.hpp"
#include "InputHandler.hpp"
#include "EventBus.hpp"
#include "RewardStrategy.hpp"
#include "AssetManager.hpp"
#include "SpriteBounds.hpp"
#include "RLConfig.hpp"
#include <algorithm>
#include <cmath>

Level1::Level1(sf::RenderWindow& window)
    : SimulationEnvironment(window), m_player(window.getSize())
{
    // Inject the Level1 reward strategy (Strategy Pattern)
    setRewardStrategy(std::make_unique<Level1RewardStrategy>());
}

// ─── Normal gameplay update ───────────────────────────────────────────────────
bool Level1::update() {
    auto winSize = m_window.getSize();
    float dt     = 1.f / 60.f;

    // Poll input via InputHandler (UI-only path — not called from RL step()).
    int  compositeAction = InputHandler::pollCompositeAction();
    int  baseDir;
    bool dashRequest, sonarReq;
    InputHandler::decodeAction(compositeAction, baseDir, dashRequest, sonarReq);

    bool dashed = m_player.applyCommand(baseDir, dashRequest);
    m_player.update(dt);
    if (dashed) getEventBus().publish({"ScreenShake", ShakeParams{DASH_SHAKE_FRAMES, DASH_SHAKE_INTENSITY}});

    if (getScore() < SCORE_SPEED_THRESHOLD)
        m_speed = INITIAL_OBSTACLE_SPEED *
                  std::pow(SCORE_SPEED_EXPONENT, getScore() / 100.f);

    float poolSpeed = m_speed * getSonarFactor();
    m_pool.update(winSize, poolSpeed);
    getEventBus().publish({"ScoreAdded", 1});

    float reward = 0.15f; // survival base

    bool died = false;
    if (!m_player.isDashing()) {
        auto& am = AssetManager::instance();
        if (m_pool.collidesWithPlayer(m_player.getSprite(), am.image("submarine"))) {
            getEventBus().publish({"ScreenShake", ShakeParams{SHAKE_FRAMES_DEATH, SHAKE_INTENSITY_DEATH}});
            getEventBus().publish({"PlayerCollision", {}});
            getEventBus().publish({"RewardPenalty", 100.f});
            reward = -100.f;
            died = true;
        }
    }

    if (reward != 0.15f) m_stepsSinceReward = 0;
    else                 ++m_stepsSinceReward;
    m_lastReward = reward;

    return died;
}

void Level1::draw() {
    m_pool.draw(m_window);
    m_player.draw(m_window);
    if (m_debugMode) {
        m_player.drawDebugHitboxes(m_window);
        m_pool.drawDebugBounds(m_window);
    }
}

// ─── RL Environment API ───────────────────────────────────────────────────────
std::vector<float> Level1::getState() const {
    auto winSize = m_window.getSize();
    float wf = static_cast<float>(winSize.x ? winSize.x : 1);
    float hf = static_cast<float>(winSize.y ? winSize.y : 1);

    sf::Vector2f ppos = m_player.getPosition();
    sf::Vector2f pvel = m_player.getVelocity();

    std::vector<float> s(55, 0.f);

    // ── Game state — s[0-10] ──────────────────────────────────────────────────
    s[0] = 0.0f;                                              // Level 1 ID
    s[1] = std::clamp(wf / RLConfig::REF_W, 0.f, 1.f);       // Viewport width
    s[2] = std::clamp(hf / RLConfig::REF_H, 0.f, 1.f);       // Viewport height
    s[3] = std::clamp(ppos.x / wf, 0.f, 1.f);               // Player x
    s[4] = std::clamp(ppos.y / hf, 0.f, 1.f);               // Player y
    s[5] = std::clamp(pvel.x / PLAYER_MAX_SPEED, -1.f, 1.f); // Player vx
    s[6] = std::clamp(pvel.y / PLAYER_MAX_SPEED, -1.f, 1.f); // Player vy
    s[7] = std::clamp(static_cast<float>(m_stepsSinceReward) / 300.f, 0.f, 1.f);  // Steps since reward
    s[8] = m_player.dashAvailable() ? 1.f : 0.f;             // Dash available
    s[9] = isSonarReady() ? 1.f : 0.f;                        // Sonar available
    s[10] = std::min(1.0f, static_cast<float>(m_idleFrames) / 100.0f);  // Idle normalized

    // ── Treasure block — s[11-14] (zeros: no treasure in Level 1) ──────────────
    s[11] = 0.f;  // treasure x
    s[12] = 0.f;  // treasure y
    s[13] = 0.f;  // treasure distance
    s[14] = 0.f;  // treasure exists (0 = no treasure)

    // ── Threat slots — s[15-54]: 8 slots × 5 features = 40 ──────────────────────
    // Enemies sorted by distance, fill slots 0-7, remainder stay 0
    auto snaps = m_pool.getSnapshots(ppos, RLConfig::MAX_OBJ_SLOTS);
    for (int i = 0; i < RLConfig::MAX_OBJ_SLOTS; ++i) {
        int base = 15 + i * 5;
        const auto& snap = snaps[i];
        bool empty = (snap.center.x == 0.f && snap.center.y == 0.f &&
                      snap.velocity.x == 0.f && snap.velocity.y == 0.f);
        
        if (empty) {
            s[base + 0] = 0.f;  // exists = 0
            // rest stay 0
        } else {
            s[base + 0] = 1.f;  // exists = 1
            s[base + 1] = std::clamp(snap.center.x   / wf, 0.f, 1.f);
            s[base + 2] = std::clamp(snap.center.y   / hf, 0.f, 1.f);
            s[base + 3] = std::clamp(snap.velocity.x / RLConfig::MAX_VEL, -1.f, 1.f);
            s[base + 4] = std::clamp(snap.velocity.y / RLConfig::MAX_VEL, -1.f, 1.f);
        }
    }

    return s;
}

std::vector<float> Level1::reset(sf::Vector2u windowSize) {
    resetScore();
    m_speed  = static_cast<float>(INITIAL_OBSTACLE_SPEED);
    m_player.reset(windowSize);
    m_pool = StarfishObstaclePool{};
    m_gameOver = false;
    return getState();
}

std::vector<float> Level1::step(int action, float& reward, bool& isDone) {
    auto winSize = m_window.getSize();
    float dt     = 1.f / 60.f;

    // Update sonar display (radius expansion)
    updateSonar(dt);

    float bonus = applyAction(m_player, action);
    m_player.setWindowSize(winSize);
    m_player.update(dt);

    if (getScore() < SCORE_SPEED_THRESHOLD)
        m_speed = static_cast<float>(INITIAL_OBSTACLE_SPEED) *
                  std::pow(SCORE_SPEED_EXPONENT, getScore() / 100.f);

    m_pool.update(winSize, m_speed);

    // Update idleness tracker
    sf::Vector2f vel = m_player.getVelocity();
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    if (speed < 0.5f) m_idleFrames++;
    else              m_idleFrames = std::max(0, m_idleFrames - 5);

    auto& am = AssetManager::instance();
    bool lethal = m_pool.collidesWithPlayer(m_player.getSprite(), am.image("submarine"));
    if (lethal) {
        getEventBus().publish({"PlayerCollision", {}});
        getEventBus().publish({"RewardPenalty", 100.f});
    }

    // ── Delegate ALL reward calculation to the injected Strategy ────────────
    EnvironmentContext ctx;
    ctx.playerSpeed  = speed;
    ctx.lethalCollision = lethal;
    ctx.idleFrames   = m_idleFrames;
    ctx.trainingMode = m_trainingMode;

    RewardResult result = m_rewardStrategy->calculateReward(ctx);
    reward = result.reward + bonus;
    isDone = result.episodeDone;

    getEventBus().publish({"ScoreAdded", 1});

    if (reward != 0.15f) m_stepsSinceReward = 0;
    else                 ++m_stepsSinceReward;
    m_lastReward = reward;

    return getState();
}

// ─── Memento API ────────────────────────────────────────────────────────────
struct Level1Snapshot : public InternalStateSnapshot {
    SimulationEnvironment::BaseSnapshot base;
    Submarine::Snapshot           player;
    StarfishObstaclePool::Snapshot      pool;
    int                                 levelScore;
    float                               speed;
};

std::unique_ptr<SimulationMemento> Level1::create_memento() const {
    auto snap = std::make_unique<Level1Snapshot>();
    snap->base       = saveBaseState();
    snap->player     = m_player.saveState();
    snap->pool       = m_pool.saveState();
    snap->levelScore = m_score;
    snap->speed      = m_speed;
    return std::make_unique<SimulationMemento>(std::move(snap));
}

void Level1::restore_memento(const SimulationMemento& memento) {
    const auto* snap = dynamic_cast<const Level1Snapshot*>(memento.m_state.get());
    if (!snap) return;

    restoreBaseState(snap->base);
    m_player.restoreState(snap->player);
    m_pool.restoreState(snap->pool, getSimSize());
    m_score = snap->levelScore;
    m_speed = snap->speed;
}

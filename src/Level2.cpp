#include "Level2.hpp"
#include "Constants.hpp"
#include "InputHandler.hpp"
#include "SpriteBounds.hpp"
#include "AssetManager.hpp"
#include "EventBus.hpp"
#include "RewardStrategy.hpp"
#include "RLConfig.hpp"
#include <algorithm>
#include <cmath>

Level2::Level2(sf::RenderWindow& window)
    : SimulationEnvironment(window)
    , m_player(window.getSize())
    , m_sine(window.getSize(), "urchin")
    , m_para(window.getSize(), "crab")
    , m_treasure(window.getSize())
{
    setRewardStrategy(std::make_unique<Level2RewardStrategy>());
}

// ─── RL Environment API Helper ──────────────────────────────────────────────────
static auto centreOf = [](sf::FloatRect b) -> sf::Vector2f {
    return { b.left + b.width * 0.5f, b.top + b.height * 0.5f };
};

// ─── Normal gameplay update ───────────────────────────────────────────────────
bool Level2::update() {
    auto  winSize = m_window.getSize();
    float dt      = 1.f / 60.f;
    float sonar   = getSonarFactor();

    int  compositeAction = InputHandler::pollCompositeAction();
    int  baseDir;
    bool dashRequest, sonarReq;
    InputHandler::decodeAction(compositeAction, baseDir, dashRequest, sonarReq);

    bool dashed = m_player.applyCommand(baseDir, dashRequest);
    m_player.update(dt);
    if (dashed) getEventBus().publish({"ScreenShake", ShakeParams{DASH_SHAKE_FRAMES, DASH_SHAKE_INTENSITY}});

    m_sine.setSpeedMultiplier(sonar);
    m_para.setSpeedMultiplier(sonar);
    m_sine.update(winSize);
    m_para.update(winSize);

    sf::Vector2f pC = centreOf(m_player.getBounds());
    sf::Vector2f tC = centreOf(m_treasure.getBounds());
    float dx = pC.x - tC.x,  dy = pC.y - tC.y;
    float curDist = std::sqrt(dx*dx + dy*dy);

    float reward = 0.15f;
    if (m_prevDistToTreasure > 0.f) reward += (m_prevDistToTreasure - curDist) * 0.50f;
    m_prevDistToTreasure = curDist;

    sf::Vector2f vel = m_player.getVelocity();
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    if (speed < 0.5f) reward -= 0.5f;

    if (m_player.getBounds().intersects(m_treasure.getBounds())) {
        m_treasure.respawn(winSize);
        getEventBus().publish({"TreasureCollected", {}});
        getEventBus().publish({"ScoreAdded", 10});
        getEventBus().publish({"RewardGranted", 100.f});
        reward += 100.f;
        m_prevDistToTreasure = -1.f;
    }

    int gs = checkGraze(m_player.getGrazeBounds(), m_player.getBounds(), m_sine.getBounds());
    gs    += checkGraze(m_player.getGrazeBounds(), m_player.getBounds(), m_para.getBounds());
    if (gs > 0) getEventBus().publish({"GrazeScored", GRAZE_SCORE_PER_FRAME * gs});

    bool died = false;
    if (!m_player.isDashing()) {
        auto& am = AssetManager::instance();
        if (pixelPerfectOverlap(m_player.getSprite(), am.image("submarine"),
                                m_sine.getSprite(),   am.image("urchin"))   ||
            pixelPerfectOverlap(m_player.getSprite(), am.image("submarine"),
                                m_para.getSprite(),   am.image("crab")))    {
            getEventBus().publish({"ScreenShake", ShakeParams{SHAKE_FRAMES_DEATH, SHAKE_INTENSITY_DEATH}});
            getEventBus().publish({"PlayerCollision", {}});
            getEventBus().publish({"RewardPenalty", 100.f});
            reward = -100.f;
            died = true;
        }
    }

    m_stepsSinceReward = (reward > 0.f) ? 0 : m_stepsSinceReward + 1;
    m_lastReward = reward;
    return died;
}

void Level2::draw() {
    m_treasure.draw(m_window);
    m_sine.draw(m_window);
    m_para.draw(m_window);
    m_player.draw(m_window);

    if (m_debugMode) {
        auto& am = AssetManager::instance();
        m_player.drawDebugHitboxes(m_window);
        drawDebugHull(m_window, m_player.getSprite(), am.hullUV("submarine"),
                      sf::Color(80, 255, 80, 220));
        drawDebugHull(m_window, m_sine.getSprite(),   am.hullUV("urchin"),
                      sf::Color(255, 80, 80, 220));
        drawDebugHull(m_window, m_para.getSprite(),   am.hullUV("crab"),
                      sf::Color(255, 80, 80, 220));
    }
}

// ─── RL Environment API ───────────────────────────────────────────────────────
std::vector<float> Level2::getState() const {
    auto winSize = m_window.getSize();
    float wf = static_cast<float>(winSize.x ? winSize.x : 1);
    float hf = static_cast<float>(winSize.y ? winSize.y : 1);

    sf::Vector2f ppos  = m_player.getPosition();
    sf::Vector2f pvel  = m_player.getVelocity();
    sf::Vector2f sC    = centreOf(m_sine.getBounds());
    sf::Vector2f pC    = centreOf(m_para.getBounds());
    sf::Vector2f tC    = centreOf(m_treasure.getBounds());
    sf::Vector2f sV    = m_sine.getVelocity();
    sf::Vector2f pV    = m_para.getVelocity();
    float diag         = std::sqrt(wf * wf + hf * hf);

    std::vector<float> s(55, 0.f);

    // ── Game state — s[0-10] ──────────────────────────────────────────────────
    s[0] = 0.5f;                                              // Level 2 ID
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

    // ── Treasure block — s[11-14] (dedicated, always present in Level 2) ──────
    float tdx = tC.x - ppos.x, tdy = tC.y - ppos.y;
    s[11] = std::clamp(tC.x / wf, 0.f, 1.f);                 // Treasure x
    s[12] = std::clamp(tC.y / hf, 0.f, 1.f);                 // Treasure y
    s[13] = std::clamp(std::sqrt(tdx*tdx + tdy*tdy) / diag, 0.f, 1.f);  // Treasure distance
    s[14] = 1.f;                                              // Treasure exists flag

    // ── Threat slots — s[15-54]: 8 slots × 5 features = 40 ──────────────────────
    // Sort urchin and crab by distance, fill slots 0-1, rest stay 0
    struct ThreatEntry {
        sf::Vector2f pos, vel;
        float dist;
    };
    std::vector<ThreatEntry> threats = {
        {sC, sV, std::sqrt((sC.x - ppos.x) * (sC.x - ppos.x) + (sC.y - ppos.y) * (sC.y - ppos.y))},
        {pC, pV, std::sqrt((pC.x - ppos.x) * (pC.x - ppos.x) + (pC.y - ppos.y) * (pC.y - ppos.y))},
    };
    std::sort(threats.begin(), threats.end(),
        [](const ThreatEntry& a, const ThreatEntry& b) { return a.dist < b.dist; });

    for (int i = 0; i < (int)threats.size(); ++i) {
        int base = 15 + i * 5;
        s[base + 0] = 1.f;  // exists
        s[base + 1] = std::clamp(threats[i].pos.x / wf, 0.f, 1.f);
        s[base + 2] = std::clamp(threats[i].pos.y / hf, 0.f, 1.f);
        s[base + 3] = std::clamp(threats[i].vel.x / RLConfig::MAX_VEL, -1.f, 1.f);
        s[base + 4] = std::clamp(threats[i].vel.y / RLConfig::MAX_VEL, -1.f, 1.f);
    }
    // Slots 2–7 stay 0 (consistent empty padding)

    return s;
}

std::vector<float> Level2::reset(sf::Vector2u windowSize) {
    resetScore();
    m_player.reset(windowSize);
    m_sine.reset(windowSize);
    m_para.reset(windowSize);
    m_treasure.respawn(windowSize);
    m_gameOver = false;
    return getState();
}

std::vector<float> Level2::step(int action, float& reward, bool& isDone) {
    auto  winSize = getSimSize();
    float dt      = 1.f / 60.f;

    // Update sonar display (radius expansion)
    updateSonar(dt);

    float bonus = applyAction(m_player, action);
    m_player.setWindowSize(winSize);
    m_player.update(dt);
    m_sine.update(winSize);
    m_para.update(winSize);

    isDone = false;

    sf::Vector2f pC = centreOf(m_player.getBounds());
    sf::Vector2f tC = centreOf(m_treasure.getBounds());
    float dx = pC.x - tC.x,  dy = pC.y - tC.y;
    float curDist = std::sqrt(dx*dx + dy*dy);

    sf::Vector2f vel = m_player.getVelocity();
    float speed = std::sqrt(vel.x * vel.x + vel.y * vel.y);
    if (speed < 0.5f) m_idleFrames++;
    else              m_idleFrames = std::max(0, m_idleFrames - 5);

    bool treasureCollected = m_player.getBounds().intersects(m_treasure.getBounds());
    if (treasureCollected) {
        m_treasure.respawn(winSize);
        getEventBus().publish({"TreasureCollected", {}});
        getEventBus().publish({"ScoreAdded", 10});
        getEventBus().publish({"RewardGranted", 100.f});
        m_prevDistToTreasure = -1.f;
    }

    auto& am = AssetManager::instance();
    bool lethal = pixelPerfectOverlap(m_player.getSprite(), am.image("submarine"),
                                      m_sine.getSprite(),   am.image("urchin")) ||
                  pixelPerfectOverlap(m_player.getSprite(), am.image("submarine"),
                                      m_para.getSprite(),   am.image("crab"));
    if (lethal) {
        getEventBus().publish({"PlayerCollision", {}});
        getEventBus().publish({"RewardPenalty", 100.f});
    }

    // ── Delegate ALL reward calculation to the injected Strategy ────────────
    EnvironmentContext ctx;
    ctx.playerSpeed         = speed;
    ctx.treasureCollected   = treasureCollected;
    ctx.lethalCollision     = lethal;
    ctx.distToTreasure      = curDist;
    ctx.prevDistToTreasure  = m_prevDistToTreasure;
    ctx.idleFrames          = m_idleFrames;
    ctx.trainingMode        = m_trainingMode;

    if (!treasureCollected) m_prevDistToTreasure = curDist;

    RewardResult result = m_rewardStrategy->calculateReward(ctx);
    reward  = result.reward + bonus;
    isDone |= result.episodeDone;

    m_stepsSinceReward = (reward > 0.f) ? 0 : m_stepsSinceReward + 1;
    m_lastReward = reward;

    return getState();
}

// ─── Memento API ────────────────────────────────────────────────────────────
struct Level2Snapshot : public InternalStateSnapshot {
    SimulationEnvironment::BaseSnapshot base;
    Submarine::Snapshot           player;
    SineObstacle::Snapshot              sine;
    ParabolicObstacle::Snapshot         para;
    Treasure::Snapshot                  treasure;
    float                               prevDistToTreasure;
};

std::unique_ptr<SimulationMemento> Level2::create_memento() const {
    auto snap = std::make_unique<Level2Snapshot>();
    snap->base               = saveBaseState();
    snap->player             = m_player.saveState();
    snap->sine               = m_sine.saveState();
    snap->para               = m_para.saveState();
    snap->treasure           = m_treasure.saveState();
    snap->prevDistToTreasure = m_prevDistToTreasure;
    return std::make_unique<SimulationMemento>(std::move(snap));
}

void Level2::restore_memento(const SimulationMemento& memento) {
    const auto* snap = dynamic_cast<const Level2Snapshot*>(memento.m_state.get());
    if (!snap) return;

    restoreBaseState(snap->base);
    m_player.restoreState(snap->player);
    m_sine.restoreState(snap->sine, getSimSize());
    m_para.restoreState(snap->para, getSimSize());
    m_treasure.restoreState(snap->treasure);
    m_prevDistToTreasure = snap->prevDistToTreasure;
}

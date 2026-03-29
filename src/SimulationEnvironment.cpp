#include "SimulationEnvironment.hpp"

#include "Submarine.hpp"
#include "AssetManager.hpp"
#include "Constants.hpp"
#include "Settings.hpp"
#include "HumanTrainer.hpp"
#include "InputHandler.hpp"
#include "EventBus.hpp"
#include "GameObservers.hpp"
#include <cstdlib>
#include <cmath>
#include <string>

// ─── Helper: get the current logical view size ────────────────────────────────
// Always use this instead of m_window.getSize() for world-space calculations.
static sf::Vector2f viewSize(const sf::RenderWindow& w) {
    return w.getView().getSize();
}

SimulationEnvironment::SimulationEnvironment(sf::RenderWindow& window) : m_window(window) {
    // ── Initialise base view from the ACTUAL current window size
    auto sz     = m_window.getSize();
    m_baseView  = sf::View(sf::FloatRect(0.f, 0.f,
                                         static_cast<float>(sz.x),
                                         static_cast<float>(sz.y)));
    m_window.setView(m_baseView);

    auto& font = AssetManager::instance().font();
    m_scoreText.setFont(font);
    m_scoreText.setFillColor(sf::Color::White);
    m_scoreText.setOutlineColor(sf::Color(0, 0, 80));
    m_scoreText.setOutlineThickness(2.f);

    m_grazeText.setFont(font);
    m_grazeText.setFillColor(sf::Color(255, 230, 60));
    m_grazeText.setOutlineColor(sf::Color(80, 60, 0));
    m_grazeText.setOutlineThickness(1.5f);

    m_pauseText.setFont(font);
    m_pauseText.setFillColor(sf::Color::White);
    m_pauseText.setOutlineColor(sf::Color::Black);
    m_pauseText.setOutlineThickness(3.f);
    m_pauseText.setString("PAUSED\nPress P to resume | ESC to quit");

    // ── Wire up EventBus observers ───────────────────────────────────────────
    // ScoreObserver: reacts to "ScoreAdded" and "GrazeScored"
    m_scoreObs = std::make_unique<ScoreObserver>(m_score, m_grazeAcc);
    m_bus.subscribe("ScoreAdded",  m_scoreObs.get());
    m_bus.subscribe("GrazeScored", m_scoreObs.get());

    // RewardObserver: reacts to "RewardGranted" and "RewardPenalty"
    m_rewardObs = std::make_unique<RewardObserver>(m_lastReward, m_stepsSinceReward, m_lastReward);
    m_bus.subscribe("RewardGranted", m_rewardObs.get());
    m_bus.subscribe("RewardPenalty", m_rewardObs.get());

    // ShakeObserver: reacts to "ScreenShake" and fires triggerShake()
    m_shakeObs = std::make_unique<ShakeObserver>(
        [this](int f, float i) { triggerShake(f, i); }
    );
    m_bus.subscribe("ScreenShake", m_shakeObs.get());

    // TelemetryObserver: diagnostics — subscribe to key events
    m_telemetryObs = std::make_unique<TelemetryObserver>();
    m_bus.subscribe("PlayerCollision",   m_telemetryObs.get());
    m_bus.subscribe("TreasureCollected", m_telemetryObs.get());
}

// ─── Main game loop ───────────────────────────────────────────────────────────
int SimulationEnvironment::run() {
    sf::Clock frameClock;

    while (m_window.isOpen() && !m_gameOver) {
        handleEvents();
        if (!m_window.isOpen()) break;

        if (m_paused) {
            m_window.clear();
            drawBackground();
            draw();
            showPauseOverlay();
            m_window.display();
            continue;
        }

        float dt = frameClock.restart().asSeconds();
        dt = std::min(dt, 0.05f);

        // ─── FPS Tracking ──────────────────────────────────────────────────
        ++m_frameMeasurementCount;
        if (m_fpsMeasurementClock.getElapsedTime().asSeconds() >= 0.5f) {
            m_currentFps = static_cast<float>(m_frameMeasurementCount) / m_fpsMeasurementClock.restart().asSeconds();
            m_frameMeasurementCount = 0;
        }

        // ─── Sonar radius update ───────────────────────────────────────────
        if (m_sonarActive) {
            m_sonarRadius += SONAR_EXPAND_SPEED * dt;
            if (m_sonarRadius >= SONAR_MAX_RADIUS) {
                m_sonarActive = false;
                m_sonarRadius = 0.f;
                m_sonarFired  = true;
                m_sonarCooldownClock.restart();
            }
        }

        m_window.clear();
        applyShake();
        drawBackground();

        bool train = Settings::instance().isTrainOnPlay() && !m_trainingMode;
        std::vector<float> state;
        int legacyAction = -1;
        if (train) {
            state = getState();
            legacyAction = InputHandler::pollCompositeAction();
            InputHandler::pollAndQueue(m_cmdQueue);
        } else {
            // Even if not training natively, we map keyboard to queue for the game
            InputHandler::pollAndQueue(m_cmdQueue);
        }

        // Process all commands (execute pattern)
        m_cmdQueue.process(*this);

        sf::Clock logicClock;
        bool died = update();  // derived class
        m_logicTimeMs = logicClock.getElapsedTime().asSeconds() * 1000.f;

        if (train && legacyAction >= 0 && legacyAction <= 14) {
            HumanTrainer::instance().recordExperience(state, legacyAction, m_lastReward, getState(), died);
        }

        draw();                // derived class
        drawSonarRing();
        drawDebugOverlay();
        if (m_showMetrics) drawMetricsOverlay();
        drawHUD();
        drawGrazeHUD();
        restoreView();
        m_window.display();
        sf::sleep(sf::milliseconds(10));

        if (died) {
            sf::sleep(sf::milliseconds(HIT_STOP_MS));  // hit stop
            m_gameOver = true;
        }
    }

    showGameOver();
    return m_score;
}

// ─── Event handling ───────────────────────────────────────────────────────────
void SimulationEnvironment::handleEvents() {
    sf::Event event;
    while (m_window.pollEvent(event)) {
        if (event.type == sf::Event::Closed) {
            m_window.close();
            return;
        }
        if (event.type == sf::Event::Resized) {
            // Keep the view in sync with whatever the OS reports.
            unsigned w = event.size.width;
            unsigned h = event.size.height;
            m_baseView = sf::View(sf::FloatRect(0.f, 0.f, (float)w, (float)h));
            m_window.setView(m_baseView);
        }
        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::P && !m_pKeyDown) {
            m_paused   = !m_paused;
            m_pKeyDown = true;
        }
        if (event.type == sf::Event::KeyReleased &&
            event.key.code == sf::Keyboard::P)
            m_pKeyDown = false;

        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::Escape)
            m_gameOver = true;

        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::F3)
            m_debugMode = !m_debugMode;

        // Metrics toggle (F4)
        if (event.type == sf::Event::KeyPressed &&
            event.key.code == sf::Keyboard::F4 && !m_f4KeyDown) {
            m_showMetrics = !m_showMetrics;
            m_f4KeyDown   = true;
        }
        if (event.type == sf::Event::KeyReleased &&
            event.key.code == sf::Keyboard::F4)
            m_f4KeyDown = false;
    }
}

// ─── Screen shake ─────────────────────────────────────────────────────────────
void SimulationEnvironment::triggerShake(int frames, float intensity) noexcept {
    m_shakeFrames    = frames;
    m_shakeIntensity = intensity;
}

// ─── Inference rendering ────────────────────────────────────────────────
// Renders one visual frame (background, sprites, sonar, HUD) without calling
// update() or polling input.  Mirrors the render block inside run().
void SimulationEnvironment::renderFrame() {
    m_window.clear();
    applyShake();
    drawBackground();
    draw();              // virtual — actual level sprites
    drawSonarRing();
    drawDebugOverlay();
    drawHUD();
    drawGrazeHUD();
    restoreView();
    m_window.display();
}

// ─── Virtual size ─────────────────────────────────────────────────────────────
// Overrides the physics/state coordinate space without resizing the OS window.
void SimulationEnvironment::setVirtualSize(sf::Vector2u sz) noexcept {
    m_virtualSize = sz;
    m_baseView = sf::View(sf::FloatRect(
        0.f, 0.f,
        static_cast<float>(sz.x), static_cast<float>(sz.y)));
    m_window.setView(m_baseView);
}

sf::Vector2u SimulationEnvironment::getSimSize() const noexcept {
    return (m_virtualSize.x > 0 && m_virtualSize.y > 0)
               ? m_virtualSize
               : m_window.getSize();
}


// ─── ICommandTarget Implementation ────────────────────────────────────────────
void SimulationEnvironment::move(int baseDir) {
    getPlayer().applyCommand(baseDir, false);
}

void SimulationEnvironment::dash() {
    bool dashed = getPlayer().applyCommand(4, true);
    if (dashed) triggerShake(DASH_SHAKE_FRAMES, DASH_SHAKE_INTENSITY);
}

void SimulationEnvironment::triggerSonar() {
    bool canFire = !m_sonarActive &&
                   (!m_sonarFired ||
                    m_sonarCooldownClock.getElapsedTime().asSeconds() >= SONAR_COOLDOWN_SEC);
    if (canFire) {
        m_sonarActive = true;
        m_sonarRadius = 0.f;
        m_sonarCenter = m_baseView.getCenter();
        m_lastReward += 1.0f; // Sonar bonus
    }
}

void SimulationEnvironment::idle() {
    getPlayer().applyCommand(4, false); // 4 = None
}

// ─── RL / Agent Action Applicator ─────────────────────────────────────────────
float SimulationEnvironment::applyAction([[maybe_unused]] Submarine& entity, int action) noexcept {
    int  baseDir;
    bool dashReq;
    bool sonarReq;
    InputHandler::decodeAction(action, baseDir, dashReq, sonarReq);

    float preReward = m_lastReward;

    if (baseDir < 4) m_cmdQueue.push(std::make_unique<MoveCommand>(baseDir));
    else             m_cmdQueue.push(std::make_unique<IdleCommand>());

    if (sonarReq)    m_cmdQueue.push(std::make_unique<SonarCommand>());
    else if (dashReq) m_cmdQueue.push(std::make_unique<DashCommand>());

    m_cmdQueue.process(*this);

    // Any positive bonus (like the sonar +1.0f) applied during execution is returned
    // so the derived `step()` function can sum it into the step reward cleanly.
    float bonusAccrued = m_lastReward - preReward;
    m_lastReward = preReward; // restore, derivative sets it properly later
    return std::max(0.0f, bonusAccrued);
}

void SimulationEnvironment::applyShake() {
    if (m_shakeFrames <= 0) {
        m_window.setView(m_baseView);
        return;
    }
    --m_shakeFrames;
    float ox = (static_cast<float>(std::rand() % 100) / 50.f - 1.f) * m_shakeIntensity;
    float oy = (static_cast<float>(std::rand() % 100) / 50.f - 1.f) * m_shakeIntensity;
    sf::View sv = m_baseView;
    sv.setCenter(sv.getCenter() + sf::Vector2f(ox, oy));
    m_window.setView(sv);
}

void SimulationEnvironment::restoreView() {
    m_window.setView(m_baseView);
}

// ─── Sonar ───────────────────────────────────────────────────────────────────
float SimulationEnvironment::getSonarFactor() const noexcept {
    return m_sonarActive ? SONAR_SLOW_FACTOR : 1.0f;
}

bool SimulationEnvironment::isSonarReady() const noexcept {
    return !m_sonarActive &&
           (!m_sonarFired ||
            m_sonarCooldownClock.getElapsedTime().asSeconds() >= SONAR_COOLDOWN_SEC);
}

void SimulationEnvironment::drawSonarRing() {
    if (!m_sonarActive || m_sonarRadius <= 0.f) return;
    float alpha = 255.f * (1.f - m_sonarRadius / SONAR_MAX_RADIUS);

    sf::CircleShape ring(m_sonarRadius);
    ring.setOrigin(m_sonarRadius, m_sonarRadius);
    ring.setPosition(m_sonarCenter);
    ring.setFillColor(sf::Color::Transparent);
    ring.setOutlineColor(sf::Color(80, 220, 255, static_cast<sf::Uint8>(alpha)));
    ring.setOutlineThickness(3.f);
    m_window.draw(ring);

    if (m_sonarRadius > 20.f) {
        sf::CircleShape inner(m_sonarRadius - 12.f);
        inner.setOrigin(m_sonarRadius - 12.f, m_sonarRadius - 12.f);
        inner.setPosition(m_sonarCenter);
        inner.setFillColor(sf::Color::Transparent);
        inner.setOutlineColor(sf::Color(40, 160, 255, static_cast<sf::Uint8>(alpha * 0.4f)));
        inner.setOutlineThickness(1.5f);
        m_window.draw(inner);
    }
}

// ─── Graze ────────────────────────────────────────────────────────────────────
int SimulationEnvironment::checkGraze(sf::FloatRect playerGraze,
                       sf::FloatRect playerCore,
                       sf::FloatRect obstacleBox) const noexcept {
    if (playerCore.intersects(obstacleBox))  return 0;
    if (playerGraze.intersects(obstacleBox)) return 1;
    return 0;
}

// ─── Background ───────────────────────────────────────────────────────────────
// Uses view size (logical coords), NOT physical window.getSize().
void SimulationEnvironment::drawBackground() {
    sf::Vector2f vs = viewSize(m_window);
    sf::VertexArray bg(sf::Quads, 4);
    bg[0] = sf::Vertex(sf::Vector2f(0,     0    ), sf::Color(0,  15,  60));
    bg[1] = sf::Vertex(sf::Vector2f(vs.x,  0    ), sf::Color(0,  15,  60));
    bg[2] = sf::Vertex(sf::Vector2f(vs.x,  vs.y ), sf::Color(0,  60, 120));
    bg[3] = sf::Vertex(sf::Vector2f(0,     vs.y ), sf::Color(0,  60, 120));
    m_window.draw(bg);
}

// ─── HUD ─────────────────────────────────────────────────────────────────────
// All sizes and positions are proportional to the current VIEW size.
void SimulationEnvironment::drawHUD() {
    sf::Vector2f vs    = viewSize(m_window);
    float        hScale = vs.y / 900.f;   // scale factor relative to 900-unit reference

    auto& font = AssetManager::instance().font();

    // ─── Cooldown Bars ────────────────────────────────────────────────────────
    float barOuterWidth = 70.f * hScale;
    float barOuterHeight = 12.f * hScale;
    float barX = vs.x * 0.012f;
    float barOuterY_dash  = vs.y * 0.012f;
    float barOuterY_sonar = barOuterY_dash + barOuterHeight + 8.f * hScale;

    // Dash (SPACE) Cooldown
    float dashRem = getPlayer().dashCooldownRemaining();
    float dashRatio = 1.0f - (dashRem / DASH_COOLDOWN_SEC);
    if (dashRatio < 0.f) dashRatio = 0.f;
    if (dashRatio > 1.f) dashRatio = 1.f;
    
    sf::RectangleShape dashBg({barOuterWidth, barOuterHeight});
    dashBg.setPosition(barX, barOuterY_dash);
    dashBg.setFillColor(sf::Color(40, 40, 40, 200));
    dashBg.setOutlineThickness(1.f);
    dashBg.setOutlineColor(sf::Color(100, 100, 100, 200));
    m_window.draw(dashBg);

    sf::RectangleShape dashFg({barOuterWidth * dashRatio, barOuterHeight});
    dashFg.setPosition(barX, barOuterY_dash);
    dashFg.setFillColor(dashRatio >= 1.f ? sf::Color(100, 255, 100, 255) : sf::Color(255, 150, 50, 255));
    m_window.draw(dashFg);

    sf::Text dashLabel;
    dashLabel.setFont(font);
    dashLabel.setString("SPC");
    dashLabel.setCharacterSize(std::max(9u, static_cast<unsigned>(10.f * hScale)));
    dashLabel.setFillColor(sf::Color::White);
    dashLabel.setPosition(barX + 2.f, barOuterY_dash - 1.f);
    m_window.draw(dashLabel);

    // Sonar (X) Cooldown
    float sonarRem = m_sonarFired ? (SONAR_COOLDOWN_SEC - m_sonarCooldownClock.getElapsedTime().asSeconds()) : 0.0f;
    if (sonarRem < 0.f) sonarRem = 0.f;
    float sonarRatio = m_sonarActive ? 0.f : (1.0f - (sonarRem / SONAR_COOLDOWN_SEC));
    if (sonarRatio < 0.f) sonarRatio = 0.f;
    if (sonarRatio > 1.f) sonarRatio = 1.f;

    sf::RectangleShape sonarBg({barOuterWidth, barOuterHeight});
    sonarBg.setPosition(barX, barOuterY_sonar);
    sonarBg.setFillColor(sf::Color(40, 40, 40, 200));
    sonarBg.setOutlineThickness(1.f);
    sonarBg.setOutlineColor(sf::Color(100, 100, 100, 200));
    m_window.draw(sonarBg);

    sf::RectangleShape sonarFg({barOuterWidth * sonarRatio, barOuterHeight});
    sonarFg.setPosition(barX, barOuterY_sonar);
    sonarFg.setFillColor(m_sonarActive ? sf::Color(255, 50, 50, 255) : (sonarRatio >= 1.f ? sf::Color(100, 255, 255, 255) : sf::Color(255, 150, 50, 255)));
    m_window.draw(sonarFg);

    sf::Text sonarLabel;
    sonarLabel.setFont(font);
    sonarLabel.setString("X");
    sonarLabel.setCharacterSize(std::max(9u, static_cast<unsigned>(10.f * hScale)));
    sonarLabel.setFillColor(sf::Color::White);
    sonarLabel.setPosition(barX + 2.f, barOuterY_sonar - 1.f);
    m_window.draw(sonarLabel);

    // Score
    float textOffsetX = barX + barOuterWidth + 15.f * hScale;
    unsigned scoreSize = static_cast<unsigned>(28.f * hScale);
    m_scoreText.setCharacterSize(std::max(14u, scoreSize));
    m_scoreText.setString("Score: " + std::to_string(m_score));
    m_scoreText.setPosition(textOffsetX, vs.y * 0.012f);
    m_window.draw(m_scoreText);

    // Sonar status (below score)
    unsigned smallSize = static_cast<unsigned>(20.f * hScale);
    smallSize = std::max(11u, smallSize);

    sf::Text sonarText;
    sonarText.setFont(font);
    sonarText.setCharacterSize(smallSize);

    bool sonarReady = !m_sonarActive &&
                      (!m_sonarFired ||
                       m_sonarCooldownClock.getElapsedTime().asSeconds() >= SONAR_COOLDOWN_SEC);
    if (sonarReady) {
        sonarText.setFillColor(sf::Color(80, 255, 200, 210));
        sonarText.setString("X Sonar: READY");
    } else if (m_sonarFired && !m_sonarActive) {
        float rem = SONAR_COOLDOWN_SEC - m_sonarCooldownClock.getElapsedTime().asSeconds();
        sonarText.setFillColor(sf::Color(80, 200, 255, 210));
        sonarText.setString("X Sonar: " + std::to_string(static_cast<int>(rem) + 1) + "s");
    } else {
        sonarText.setFillColor(sf::Color(80, 200, 255, 255));
        sonarText.setString("X Sonar: ACTIVE");
    }
    sonarText.setPosition(textOffsetX, vs.y * 0.012f + scoreSize + 4.f);
    m_window.draw(sonarText);

    // F3 debug hint — top-right corner
    sf::Text dbgHint;
    dbgHint.setFont(font);
    dbgHint.setCharacterSize(std::max(11u, static_cast<unsigned>(14.f * hScale)));
    if (m_debugMode) {
        dbgHint.setFillColor(sf::Color(255, 80, 80, 220));
        dbgHint.setString("DEBUG  [F3 off]");
    } else {
        dbgHint.setFillColor(sf::Color(120, 180, 180, 130));
        dbgHint.setString("[F3] debug");
    }
    sf::FloatRect db = dbgHint.getLocalBounds();
    dbgHint.setOrigin(db.width, 0.f);
    dbgHint.setPosition(vs.x - vs.x * 0.012f, vs.y * 0.012f);
    m_window.draw(dbgHint);

    // Controls reminder — bottom-centre
    sf::Text ctrl;
    ctrl.setFont(font);
    ctrl.setCharacterSize(std::max(11u, static_cast<unsigned>(14.f * hScale)));
    ctrl.setFillColor(sf::Color(120, 180, 220, 110));
    ctrl.setString("SPACE: dash   X: sonar   P: pause   ESC: menu");
    sf::FloatRect cb = ctrl.getLocalBounds();
    ctrl.setOrigin(cb.width / 2.f, cb.height);
    ctrl.setPosition(vs.x / 2.f, vs.y - vs.y * 0.012f);
    m_window.draw(ctrl);
}

void SimulationEnvironment::drawGrazeHUD() {
    if (m_grazeAcc == 0) return;
    sf::Vector2f vs    = viewSize(m_window);
    float        hScale = vs.y / 900.f;
    m_grazeText.setCharacterSize(std::max(13u, static_cast<unsigned>(20.f * hScale)));
    m_grazeText.setString("GRAZE  +" + std::to_string(m_grazeAcc));
    sf::FloatRect gb = m_grazeText.getLocalBounds();
    m_grazeText.setOrigin(gb.width / 2.f, 0.f);
    m_grazeText.setPosition(vs.x / 2.f, vs.y * 0.012f);
    m_window.draw(m_grazeText);
}

// ─── Debug overlay ────────────────────────────────────────────────────────────
void SimulationEnvironment::drawDebugOverlay() {
    // Hitbox drawing is delegated to Player::drawDebugHitboxes() and
    // per-level obstacle debug code, called from the derived draw().
    // Nothing extra needed here.
}

// ─── Pause overlay ────────────────────────────────────────────────────────────
void SimulationEnvironment::showPauseOverlay() {
    sf::Vector2f vs    = viewSize(m_window);
    float        hScale = vs.y / 900.f;

    sf::RectangleShape dim(vs);
    dim.setFillColor(sf::Color(0, 0, 0, 160));
    m_window.draw(dim);

    m_pauseText.setCharacterSize(std::max(24u, static_cast<unsigned>(52.f * hScale)));
    sf::FloatRect pb = m_pauseText.getLocalBounds();
    m_pauseText.setOrigin(pb.width / 2.f, pb.height / 2.f);
    m_pauseText.setPosition(vs.x / 2.f, vs.y / 2.f);
    m_window.draw(m_pauseText);
    m_window.display();
}

// ─── Game over ────────────────────────────────────────────────────────────────
void SimulationEnvironment::showGameOver() {
    if (!m_window.isOpen()) return;
    triggerShake(SHAKE_FRAMES_DEATH, SHAKE_INTENSITY_DEATH);

    auto&    font   = AssetManager::instance().font();
    sf::Text goText;
    goText.setFont(font);
    goText.setFillColor(sf::Color(255, 80, 80));
    goText.setOutlineColor(sf::Color::Black);
    goText.setOutlineThickness(3.f);
    goText.setString("Game Over!\nScore: " + std::to_string(m_score) +
                     "\nGraze bonus: " + std::to_string(m_grazeAcc));

    for (int frame = 0; frame < 120; ++frame) {
        sf::Event e;
        while (m_window.pollEvent(e)) {
            if (e.type == sf::Event::Closed) { m_window.close(); return; }
        }
        sf::Vector2f vs    = viewSize(m_window);
        float        hScale = vs.y / 900.f;
        goText.setCharacterSize(std::max(24u, static_cast<unsigned>(52.f * hScale)));

        m_window.clear();
        applyShake();
        drawBackground();
        sf::FloatRect tb = goText.getLocalBounds();
        goText.setOrigin(tb.width / 2.f, tb.height / 2.f);
        goText.setPosition(vs.x / 2.f, vs.y / 2.f);
        m_window.draw(goText);
        restoreView();
        m_window.display();
        sf::sleep(sf::milliseconds(16));
    }
}

// ─── Metrics Overlay ──────────────────────────────────────────────────────────
void SimulationEnvironment::drawMetricsOverlay() {
    sf::Vector2f vs = m_baseView.getSize();

    sf::Text txt;
    txt.setFont(AssetManager::instance().font());
    txt.setCharacterSize(std::max(12u, static_cast<unsigned>(vs.y * 0.025f)));
    txt.setFillColor(sf::Color(100, 255, 100));
    txt.setOutlineColor(sf::Color::Black);
    txt.setOutlineThickness(2.f);

    char buf[128];
    snprintf(buf, sizeof(buf), "FPS: %.1f\nLogic: %.2f ms\nReward/Step: %.2f", 
             m_currentFps, m_logicTimeMs, m_lastReward);
    txt.setString(buf);

    // Position subtly on the top left, under the standard HUD
    txt.setPosition(vs.x * 0.02f, vs.y * 0.15f);
    m_window.draw(txt);
}

// ─── Base Memento API ────────────────────────────────────────────────────────
SimulationEnvironment::BaseSnapshot SimulationEnvironment::saveBaseState() const {
    BaseSnapshot snap;
    snap.stepsSinceReward     = m_stepsSinceReward;
    snap.lastReward           = m_lastReward;
    snap.idleFrames           = m_idleFrames;
    snap.paused               = m_paused;
    snap.gameOver             = m_gameOver;
    snap.score                = m_score;
    snap.grazeAcc             = m_grazeAcc;
    snap.shakeFrames          = m_shakeFrames;
    snap.shakeIntensity       = m_shakeIntensity;
    snap.sonarActive          = m_sonarActive;
    snap.sonarRadius          = m_sonarRadius;
    snap.sonarCenter          = m_sonarCenter;
    snap.sonarFired           = m_sonarFired;
    snap.sonarClockSec        = m_sonarClock.getElapsedTime().asSeconds();
    snap.sonarCooldownClockSec= m_sonarCooldownClock.getElapsedTime().asSeconds();
    return snap;
}

void SimulationEnvironment::restoreBaseState(const BaseSnapshot& snap) {
    m_stepsSinceReward = snap.stepsSinceReward;
    m_lastReward       = snap.lastReward;
    m_idleFrames       = snap.idleFrames;
    m_paused           = snap.paused;
    m_gameOver         = snap.gameOver;
    m_score            = snap.score;
    m_grazeAcc         = snap.grazeAcc;
    m_shakeFrames      = snap.shakeFrames;
    m_shakeIntensity   = snap.shakeIntensity;
    m_sonarActive      = snap.sonarActive;
    m_sonarRadius      = snap.sonarRadius;
    m_sonarCenter      = snap.sonarCenter;
    m_sonarFired       = snap.sonarFired;
    // For Memento reset, accepting slight SFML clock drift is fine since
    // it's mainly for exact physics/logic reproduction which is controlled by frame deltas.
    // The visual cooldown timer might have a sub-second desync on load but logic aligns
    // with m_sonarFired and elapsed time checks.
}

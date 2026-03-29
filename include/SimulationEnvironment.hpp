#pragma once
#include <SFML/Graphics.hpp>
#include <string>
#include <vector>
#include <functional>
#include <memory>
#include "Command.hpp"
#include "CommandQueue.hpp"
#include "EventBus.hpp"
#include "GameObservers.hpp"
#include "RewardStrategy.hpp"
#include "SimulationMemento.hpp"

class Submarine; // new entity name

// ─── Abstract SimulationEnvironment base ──────────────────────────────────────────────────────
// Provides the shared game loop, HUD, blue gradient background, pause/resume,
// screen shake, sonar pulse (X key), graze HUD, hit-stop, and debug overlay.
//
// For RL training, set isTrainingMode = true via setTrainingMode() BEFORE
// calling reset() / step().  The normal run() loop is never called in that case.
//
// Derived classes implement:
//   bool update()  — move obstacles, check collision, return true = player dead
//   void draw()    — draw enemies, player, treasure
//   std::vector<float> getState()  — 12-element normalised observation
//   std::vector<float> reset(sf::Vector2u)  — reset episode, return first obs
//   std::vector<float> step(int action, float& reward, bool& done)
class SimulationEnvironment : public ICommandTarget {
public:
    explicit SimulationEnvironment(sf::RenderWindow& window);
    virtual ~SimulationEnvironment() = default;

    // ─── Normal gameplay loop ─────────────────────────────────────────────
    // Runs until game-over or Escape. Returns final score.
    int run();

    // ─── ICommandTarget Implementation ────────────────────────────────────
    void move(int baseDir) override;
    void dash() override;
    void triggerSonar() override;
    void idle() override;

    // ─── RL training API ──────────────────────────────────────────────────
    void setTrainingMode(bool val) noexcept { m_trainingMode = val; }
    bool isTrainingMode()          const noexcept { return m_trainingMode; }

    virtual std::vector<float> getState()                                   const = 0;
    virtual std::vector<float> reset(sf::Vector2u windowSize)                     = 0;
    virtual std::vector<float> step(int action, float& reward, bool& isDone)      = 0;

    // ─── Memento Pattern (Episodic Reset API) ─────────────────────────────
    virtual std::unique_ptr<SimulationMemento> create_memento() const = 0;
    virtual void restore_memento(const SimulationMemento& memento) = 0;

    // Convenience: window size used by headless training loop
    sf::Vector2u getWindowSize() const noexcept { return m_window.getSize(); }

    // Access to player for HUD rendering of abilities
    virtual const Submarine& getPlayer() const = 0;
    virtual Submarine&       getPlayer()       = 0;

    // ─── Inference rendering ──────────────────────────────────────────────
    // Render one visual frame: background → shake → level sprites → HUD.
    // Call this from play.cpp after env->step() to show the full game view.
    void renderFrame();

    // ─── Virtual size (multi-screen training / live resize) ─────────────────
    // Override the physics size used by step() without resizing the OS window.
    // Training uses this to randomise screen sizes each episode.
    // Play mode uses it to recalibrate on window-resize events.
    void         setVirtualSize(sf::Vector2u sz) noexcept;
    sf::Vector2u getSimSize()                    const noexcept;

protected:
    // ─── Required overrides ────────────────────────────────────────────────
    virtual bool update() = 0;    // returns true → player dies → hit-stop
    virtual void draw()   = 0;    // draw level-specific objects

    // ─── RL helper ────────────────────────────────────────────────────────
    // Translate discrete action (0=Up,1=Down,2=Left,3=Right) into a velocity
    // impulse applied directly to the Submarine (bypasses keyboard polling).
    float applyAction(Submarine& entity, int action) noexcept;

    // ─── Event Bus access for derived classes ─────────────────────────────
    EventBus& getEventBus() noexcept { return m_bus; }

    // ─── RewardStrategy injection (Strategy Pattern) ───────────────────────
    // Derived levels call this in their constructors to install a strategy.
    // After injection, step() must produce an EnvironmentContext and invoke
    // m_rewardStrategy->calculateReward(ctx) — no reward conditionals in SimulationEnvironment.
    void setRewardStrategy(std::unique_ptr<IRewardStrategy> strategy) {
        m_rewardStrategy = std::move(strategy);
    }

    // ─── Shared helpers for subclasses ────────────────────────────────────
    void drawBackground();
    void drawHUD();
    void addScore(int amount = 1) noexcept;
    int  getScore()               const noexcept { return m_score; }
    void resetScore()             noexcept {
        m_score = 0; m_grazeAcc = 0;
        m_stepsSinceReward = 0; m_lastReward = 0.f;
        m_idleFrames = 0;
    }

    // Screen shake — trigger this when the player dashes or hits something.
    void triggerShake(int frames, float intensity) noexcept;

    // Sonar — returns the current obstacle speed multiplier (1.0 or SONAR_SLOW_FACTOR).
    float getSonarFactor() const noexcept;

    // Sonar readiness — returns true when the sonar ability can be fired.
    // Derived classes use this to populate the state vector at s[48].
    bool isSonarReady() const noexcept;

public:
    // ─── Base State Memento ───────────────────────────────────────────────
    struct BaseSnapshot {
        int   stepsSinceReward;
        float lastReward;
        int   idleFrames;
        bool  paused;
        bool  gameOver;
        int   score;
        int   grazeAcc;
        int   shakeFrames;
        float shakeIntensity;
        bool  sonarActive;
        float sonarRadius;
        sf::Vector2f sonarCenter;
        bool  sonarFired;
        float sonarClockSec;
        float sonarCooldownClockSec;
    };
protected:
    BaseSnapshot saveBaseState() const;
    void         restoreBaseState(const BaseSnapshot& snap);

    // Graze — call from update(). Returns 1 if graze, 0 otherwise.
    // Does NOT fire if playerCore intersects obstacleBox (that's a lethal hit).
    int checkGraze(sf::FloatRect playerGraze,
                   sf::FloatRect playerCore,
                   sf::FloatRect obstacleBox) const noexcept;

    // Debug mode flag (toggled by F3 key).
    bool m_debugMode    = false;
    bool m_trainingMode = false;

    // RL reward-timing state (updated by each level's step())
    int   m_stepsSinceReward = 0;   // frames elapsed since last non-zero reward
    float m_lastReward       = 0.f; // reward value from the most recent step
    int   m_idleFrames       = 0;   // frames elapsed since last moving >= 0.5f

    // ─── Strategy Pattern: RewardStrategy (protected so derived step() can call it)
    std::unique_ptr<IRewardStrategy> m_rewardStrategy;

    sf::RenderWindow& m_window;
    bool              m_paused      = false;
    bool              m_gameOver    = false;
    sf::Vector2u      m_virtualSize = {};  // 0 = use m_window.getSize()

private:
    void handleEvents();
    void applyShake();
    void restoreView();
    void drawSonarRing();
    void drawGrazeHUD();
    void drawDebugOverlay();
    void drawMetricsOverlay();
    void showGameOver();
    void showPauseOverlay();

    // ─── Score / HUD ──────────────────────────────────────────────────────
    int      m_score    = 0;
    int      m_grazeAcc = 0;   // graze points accumulated this run
    sf::Text m_scoreText;
    sf::Text m_pauseText;
    sf::Text m_grazeText;

    // ─── Pause ────────────────────────────────────────────────────────────
    bool m_pKeyDown  = false;

    // ─── Screen shake ─────────────────────────────────────────────────────
    int   m_shakeFrames    = 0;
    float m_shakeIntensity = 0.f;
    sf::View m_baseView;

    // ─── Sonar pulse ──────────────────────────────────────────────────────
    bool         m_sonarActive   = false;
    float        m_sonarRadius   = 0.f;
    sf::Vector2f m_sonarCenter;
    sf::Clock    m_sonarClock;
    bool         m_sonarFired    = false;
    sf::Clock    m_sonarCooldownClock;
    // ─── Metrics ──────────────────────────────────────────────────────────
    bool      m_showMetrics = false;
    bool      m_f4KeyDown   = false;
    sf::Clock m_fpsMeasurementClock;
    int       m_frameMeasurementCount = 0;
    float     m_currentFps   = 0.f;
    float     m_logicTimeMs  = 0.f;

    // ─── Telemetry Queue ──────────────────────────────────────────────────
    CommandQueue m_cmdQueue{"telemetry.log"};

    // ─── Event Bus & Observer Wiring ────────────────────────────────────────
    // All observers are stored as members so they live for the SimulationEnvironment's lifetime.
    EventBus    m_bus;
    // Initialised in SimulationEnvironment constructor (after m_score / m_grazeAcc / m_lastReward
    // members are defined), so they are declared here as optional pointers and
    // constructed in SimulationEnvironment.cpp. Using unique_ptr so we can init after construction.
    std::unique_ptr<ScoreObserver>     m_scoreObs;
    std::unique_ptr<RewardObserver>    m_rewardObs;
    std::unique_ptr<ShakeObserver>     m_shakeObs;
    std::unique_ptr<TelemetryObserver> m_telemetryObs;
};

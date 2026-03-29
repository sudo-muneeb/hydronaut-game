#pragma once
// ─── IInterfaces.hpp ──────────────────────────────────────────────────────────
// Pure abstract interfaces for Hydronaut's core systems.
// All dependencies in main.cpp / GameApp are expressed exclusively via these
// interfaces — no concrete class names appear in the GameApp type or constructor.
//
// Concrete implementations are provided by:
//   • SFMLWindowAdapter    → IRenderWindow
//   • SFMLMusicAdapter     → IAudioPlayer
//   • AssetManagerAdapter  → IAssetManager
//   • SettingsAdapter      → ISettings
//   • HumanTrainerAdapter  → ITrainer
//   • SFMLMenuFactory      → IMenuFactory
//   • SFMLLevelFactory     → ILevelFactory
// These adapters live in SFMLAdapters.hpp.
// ─────────────────────────────────────────────────────────────────────────────

#include <string>
#include <memory>

// ─── IRenderWindow ────────────────────────────────────────────────────────────
// Abstracts the OS window: only the methods needed by the top-level game loop
// are exposed here.  SFML-heavy operations stay inside SimulationEnvironment / Menu internals.
class IRenderWindow {
public:
    virtual ~IRenderWindow() = default;

    // Returns true while the window has not been closed.
    virtual bool isOpen() const = 0;
};

// ─── IAudioPlayer ─────────────────────────────────────────────────────────────
// Abstracts background music playback.
class IAudioPlayer {
public:
    virtual ~IAudioPlayer() = default;

    // Load a music file from disk. Returns false on failure.
    virtual bool openFromFile(const std::string& filename) = 0;

    // Set whether the track loops when it finishes.
    virtual void setLoop(bool loop) = 0;

    // Volume in [0, 100].
    virtual void setVolume(float volume) = 0;

    // Begin playback.
    virtual void play() = 0;
};

// ─── IAssetManager ────────────────────────────────────────────────────────────
// Abstracts global asset loading.
class IAssetManager {
public:
    virtual ~IAssetManager() = default;

    // Load all textures, fonts, and pre-compute collision hulls.
    virtual void loadAll() = 0;
};

// ─── ISettings ────────────────────────────────────────────────────────────────
// Read-only view of audio / gameplay settings used by the initialization path.
class ISettings {
public:
    virtual ~ISettings() = default;

    virtual bool  isMuted()    const = 0;
    virtual float getVolume()  const = 0;
};

// ─── ITrainer ─────────────────────────────────────────────────────────────────
// Abstracts the RL trainer's lifecycle so GameApp can trigger a clean shutdown
// without depending on the concrete HumanTrainer singleton or LibTorch.
class ITrainer {
public:
    virtual ~ITrainer() = default;

    // Flush model weights to disk and release LibTorch resources.
    virtual void shutdown() = 0;
};

// ─── IMenu ────────────────────────────────────────────────────────────────────
// Abstracts the main menu: displays UI, returns the user's choice.
class IMenu {
public:
    virtual ~IMenu() = default;

    // Blocking call: runs the menu event loop and returns:
    //   1 = Level1 | 2 = Level2 | 3 = Level3 | -1 = quit / window closed
    virtual int run() = 0;
};

// ─── IMenuFactory ─────────────────────────────────────────────────────────────
// Produces fresh IMenu instances (menus are not reused across entries).
class IMenuFactory {
public:
    virtual ~IMenuFactory() = default;
    virtual std::unique_ptr<IMenu> createMenu() = 0;
};

// ─── ILevel ───────────────────────────────────────────────────────────────────
// Abstracts a playable level: runs the event+render loop until game-over/escape.
class ILevel {
public:
    virtual ~ILevel() = default;

    // Blocking call: runs the level event loop until game-over or Escape.
    // Returns the final score.
    virtual int run() = 0;
};

// ─── ILevelFactory ────────────────────────────────────────────────────────────
// Produces ILevel instances from a numeric choice.
// Returns nullptr for unsupported choices so callers can guard gracefully.
class ILevelFactory {
public:
    virtual ~ILevelFactory() = default;

    // choice: 1 = Level1, 2 = Level2, 3 = Level3
    virtual std::unique_ptr<ILevel> createLevel(int choice) = 0;
};

// ─── IGameApp ─────────────────────────────────────────────────────────────────
// Top-level application — runs the complete game session.
class IGameApp {
public:
    virtual ~IGameApp() = default;

    // Blocking: initialize systems, run the menu/level loop, and clean up.
    virtual void run() = 0;
};

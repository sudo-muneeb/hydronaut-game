#pragma once
// ─── SFMLAdapters.hpp ─────────────────────────────────────────────────────────
// Concrete adapters that implement the pure abstract interfaces defined in
// IInterfaces.hpp using real SFML / singleton objects.
//
// These are the ONLY files in the project that are allowed to #include concrete
// classes (Level1/2/3, Menu, AssetManager, Settings, HumanTrainer).
// GameApp and main() talk exclusively through IInterfaces.
// ─────────────────────────────────────────────────────────────────────────────

#include "IInterfaces.hpp"

#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>

#include "AssetManager.hpp"
#include "Settings.hpp"
#include "HumanTrainer.hpp"
#include "Menu.hpp"
#include "Level1.hpp"
#include "Level2.hpp"
#include "Level3.hpp"

#include <memory>
#include <stdexcept>

// ─── SFMLWindowAdapter ────────────────────────────────────────────────────────
// Wraps a raw sf::RenderWindow reference so GameApp never touches the concrete
// type. The window is created in main() (the composition root) and lives there
// for the entire program lifetime.
class SFMLWindowAdapter final : public IRenderWindow {
public:
    explicit SFMLWindowAdapter(sf::RenderWindow& window) : m_window(window) {}

    bool isOpen() const override { return m_window.isOpen(); }

private:
    sf::RenderWindow& m_window;
};

// ─── SFMLMusicAdapter ─────────────────────────────────────────────────────────
// Wraps an sf::Music object. Owns its own Music instance.
class SFMLMusicAdapter final : public IAudioPlayer {
public:
    bool openFromFile(const std::string& filename) override {
        return m_music.openFromFile(filename);
    }
    void setLoop(bool loop)       override { m_music.setLoop(loop); }
    void setVolume(float volume)  override { m_music.setVolume(volume); }
    void play()                   override { m_music.play(); }

    // Expose the underlying SFML object for UI controls in the composition root.
    sf::Music* rawHandle() { return &m_music; }

private:
    sf::Music m_music;
};

// ─── AssetManagerAdapter ──────────────────────────────────────────────────────
// Delegates to the AssetManager singleton. The adapter itself is injected so
// GameApp never calls AssetManager::instance() directly.
class AssetManagerAdapter final : public IAssetManager {
public:
    void loadAll() override { AssetManager::instance().loadAll(); }
};

// ─── SettingsAdapter ──────────────────────────────────────────────────────────
// Delegates to the Settings singleton. GameApp sees only immutable ISettings.
class SettingsAdapter final : public ISettings {
public:
    bool  isMuted()   const override { return Settings::instance().isMuted(); }
    float getVolume() const override { return Settings::instance().getVolume(); }
};

// ─── HumanTrainerAdapter ──────────────────────────────────────────────────────
// Delegates to the HumanTrainer singleton. ITrainer exposes only shutdown(),
// keeping the RL API fully hidden from the game loop.
class HumanTrainerAdapter final : public ITrainer {
public:
    void shutdown() override { HumanTrainer::instance().shutdown(); }
};

// ─── SFMLMenuAdapter ──────────────────────────────────────────────────────────
// Wraps a concrete Menu instance and exposes it as IMenu.
// Created fresh per call to SFMLMenuFactory::createMenu() so the menu state
// is clean every time the player returns to the main screen.
class SFMLMenuAdapter final : public IMenu {
public:
    // The concrete Menu needs the underlying sf::RenderWindow and optionally
    // an sf::Music pointer (for volume controls). These are injected via the
    // factory below rather than wired here directly.
    explicit SFMLMenuAdapter(sf::RenderWindow& window, sf::Music* music)
        : m_menu(window, music) {}

    int run() override { return m_menu.run(); }

private:
    Menu m_menu;
};

// ─── SFMLMenuFactory ──────────────────────────────────────────────────────────
// IMenuFactory concrete implementation. Owns the underlying sf::Music pointer
// so the Music object lifetimes are managed in one place (main.cpp).
class SFMLMenuFactory final : public IMenuFactory {
public:
    // window  — the OS render surface (always valid while GameApp::run() runs)
    // music   — nullable; Menu uses it to adjust volume from the settings panel
    SFMLMenuFactory(sf::RenderWindow& window, sf::Music* music)
        : m_window(window), m_music(music) {}

    std::unique_ptr<IMenu> createMenu() override {
        return std::make_unique<SFMLMenuAdapter>(m_window, m_music);
    }

private:
    sf::RenderWindow& m_window;
    sf::Music*        m_music;   // nullable — may be nullptr if music failed to load
};

// ─── SFMLLevelAdapter ─────────────────────────────────────────────────────────
// Wraps any concrete SimulationEnvironment subclass (Level1/2/3) and exposes it as ILevel.
// SimulationEnvironment already has a virtual run(), so the adapter is a thin forwarder.
class SFMLLevelAdapter final : public ILevel {
public:
    explicit SFMLLevelAdapter(std::unique_ptr<SimulationEnvironment> level)
        : m_level(std::move(level)) {}

    int run() override { return m_level->run(); }

private:
    std::unique_ptr<SimulationEnvironment> m_level;
};

// ─── SFMLLevelFactory ─────────────────────────────────────────────────────────
// ILevelFactory concrete implementation. Instantiates the correct SimulationEnvironment subclass
// for the choice returned by IMenu::run().
class SFMLLevelFactory final : public ILevelFactory {
public:
    // window — injected so SimulationEnvironment subclass constructors receive it via DI.
    explicit SFMLLevelFactory(sf::RenderWindow& window) : m_window(window) {}

    // Returns nullptr for unsupported choices; callers guard with `if (level)`.
    std::unique_ptr<ILevel> createLevel(int choice) override {
        std::unique_ptr<SimulationEnvironment> concrete;
        switch (choice) {
            case 1: concrete = std::make_unique<Level1>(m_window); break;
            case 2: concrete = std::make_unique<Level2>(m_window); break;
            case 3: concrete = std::make_unique<Level3>(m_window); break;
            default: return nullptr;
        }
        return std::make_unique<SFMLLevelAdapter>(std::move(concrete));
    }

private:
    sf::RenderWindow& m_window;
};

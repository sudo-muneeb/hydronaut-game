#pragma once
// ─── GameApp.hpp ──────────────────────────────────────────────────────────────
// Encapsulates the core Hydronaut game loop with ZERO concrete class coupling.
// All dependencies are injected via the constructor as pure abstract interfaces
// defined in IInterfaces.hpp. This class never includes Level1.hpp, Menu.hpp,
// AssetManager.hpp, HumanTrainer.hpp, or any SFML header.
// ─────────────────────────────────────────────────────────────────────────────

#include "IInterfaces.hpp"
#include <memory>
#include <iostream>
#include <stdexcept>

class GameApp final : public IGameApp {
public:
    // ─── Constructor Dependency Injection ────────────────────────────────────
    // Every dependency is handed in as an abstract interface.
    // The GameApp owns the unique_ptr services and shares the shared_ptr ones.
    GameApp(
        std::unique_ptr<IRenderWindow> window,
        std::unique_ptr<IAudioPlayer>  audio,
        std::shared_ptr<IAssetManager> assetManager,
        std::shared_ptr<ISettings>     settings,
        std::shared_ptr<ITrainer>      trainer,
        std::unique_ptr<IMenuFactory>  menuFactory,
        std::unique_ptr<ILevelFactory> levelFactory,
        const std::string&             musicFilePath
    )
        : m_window      (std::move(window))
        , m_audio       (std::move(audio))
        , m_assetManager(std::move(assetManager))
        , m_settings    (std::move(settings))
        , m_trainer     (std::move(trainer))
        , m_menuFactory (std::move(menuFactory))
        , m_levelFactory(std::move(levelFactory))
        , m_musicFilePath(musicFilePath)
    {}

    // ─── run() ───────────────────────────────────────────────────────────────
    // Initializes all systems, runs the main menu→level loop, then shuts down.
    // No concrete type appears anywhere inside this method body.
    void run() override {
        // 1. Load all game assets (textures, fonts, collision hulls).
        m_assetManager->loadAll();

        // 2. Initialize background music.
        bool musicOk = m_audio->openFromFile(m_musicFilePath);
        if (musicOk) {
            m_audio->setLoop(true);
            m_audio->setVolume(m_settings->isMuted() ? 0.f : m_settings->getVolume());
            m_audio->play();
        } else {
            std::cerr << "[WARNING] Music file not found: " << m_musicFilePath << "\n";
        }

        // 3. Core game loop: menu → level → menu → …
        while (m_window->isOpen()) {
            // Create a fresh menu via the injected factory (no concrete Menu here).
            auto menu   = m_menuFactory->createMenu();
            int  choice = menu->run();

            // -1  → Escape / window closed → exit the loop.
            if (choice == -1 || !m_window->isOpen())
                break;

            // Create the chosen level through the injected factory.
            auto level = m_levelFactory->createLevel(choice);
            if (level) {
                try {
                    level->run();
                } catch (const std::exception& e) {
                    std::cerr << "[ERROR] SimulationEnvironment crashed: " << e.what() << "\n";
                }
            }
        }

        // 4. Flush RL model weights and release LibTorch resources before exit.
        m_trainer->shutdown();
    }

private:
    // ─── Injected dependencies (purely interface-typed) ───────────────────────
    std::unique_ptr<IRenderWindow> m_window;
    std::unique_ptr<IAudioPlayer>  m_audio;
    std::shared_ptr<IAssetManager> m_assetManager;
    std::shared_ptr<ISettings>     m_settings;
    std::shared_ptr<ITrainer>      m_trainer;
    std::unique_ptr<IMenuFactory>  m_menuFactory;
    std::unique_ptr<ILevelFactory> m_levelFactory;

    std::string m_musicFilePath;
};

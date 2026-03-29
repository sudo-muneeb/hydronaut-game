#include <SFML/Graphics.hpp>
#include <SFML/Audio.hpp>
#include <iostream>
#include <memory>
#include <stdexcept>

// ─── Interfaces & Adapters ────────────────────────────────────────────────────
// IInterfaces.hpp  — pure abstract base classes (no SFML / concrete deps)
// SFMLAdapters.hpp — concrete adapter implementations (the ONLY place concrete
//                    classes such as Level1, Menu, AssetManager are #included)
// GameApp.hpp      — fully decoupled game-loop class (depends only on interfaces)
#include "Constants.hpp"
#include "IInterfaces.hpp"
#include "SFMLAdapters.hpp"
#include "GameApp.hpp"

// ─── Composition Root ─────────────────────────────────────────────────────────
// main() is the ONLY function in the project permitted to name concrete types.
// Its sole responsibility is to wire up the dependency graph and hand control
// to the abstract IGameApp::run().
int main() {
    try {
        // ── 1. Create the OS window (concrete SFML type lives only here) ──
        sf::VideoMode desktop = sf::VideoMode::getDesktopMode();

        auto sfmlWindow = std::make_unique<sf::RenderWindow>(
            desktop,
            "Hydronaut",
            sf::Style::Default
        );
        sfmlWindow->setFramerateLimit(60);
        sfmlWindow->setView(sf::View(sf::FloatRect(
            0.f, 0.f,
            static_cast<float>(desktop.width),
            static_cast<float>(desktop.height))));

        // Keep a raw reference so the factories can forward it to Levels/Menus
        // without the adapters needing to own the window.
        sf::RenderWindow& windowRef = *sfmlWindow;

        // ── 2. Build the SFMLMusicAdapter and get the underlying sf::Music* ─
        // SFMLMenuFactory needs a nullable sf::Music* for the settings panel;
        // we expose it via a second raw pointer after construction. We declare
        // the adapter first so its lifetime covers GameApp::run().
        auto musicAdapter = std::make_unique<SFMLMusicAdapter>();
        // Composition root may access concrete adapter internals for wiring only.
        sf::Music* rawMusic = musicAdapter->rawHandle();

        // ── 3. Assemble the dependency graph via abstract interfaces ─────────

        // window adapter
        auto windowAdapter  = std::make_unique<SFMLWindowAdapter>(windowRef);

        // shared stateless services
        auto assetManager   = std::make_shared<AssetManagerAdapter>();
        auto settings       = std::make_shared<SettingsAdapter>();
        auto trainer        = std::make_shared<HumanTrainerAdapter>();

        // factories (need the raw window/music for forwarding to concrete ctors)
        // Pass the same music object used by GameApp so menu settings immediately
        // update the currently playing track.
        auto menuFactory    = std::make_unique<SFMLMenuFactory>(windowRef, rawMusic);
        auto levelFactory   = std::make_unique<SFMLLevelFactory>(windowRef);

        // ── 4. Construct GameApp — no concrete type appears in this call ──────
        GameApp app(
            std::move(windowAdapter),
            std::move(musicAdapter),
            assetManager,
            settings,
            trainer,
            std::move(menuFactory),
            std::move(levelFactory),
            std::string(ASSET_MUSIC)   // music file path injected as plain string
        );

        // ── 5. Run — all logic is now inside the decoupled GameApp ────────────
        app.run();

    } catch (const std::exception& e) {
        std::cerr << "[FATAL] " << e.what() << "\n";
        return EXIT_FAILURE;
    } catch (...) {
        std::cerr << "[FATAL] Unknown exception.\n";
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

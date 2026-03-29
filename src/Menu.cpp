#include "Menu.hpp"
#include "AssetManager.hpp"
#include "Constants.hpp"
#include "Settings.hpp"
#include <string>

// ─── Helper: logical view size ────────────────────────────────────────────────
static sf::Vector2f vSize(const sf::RenderWindow& w) {
    return w.getView().getSize();
}

Menu::Menu(sf::RenderWindow& window, sf::Music* music)
    : m_window(window), m_music(music), m_font(AssetManager::instance().font())
{
    auto sz = m_window.getSize();
    m_window.setView(sf::View(sf::FloatRect(
        0.f, 0.f, static_cast<float>(sz.x), static_cast<float>(sz.y))));

    for (int i = 0; i < MAX_ITEMS; ++i) {
        m_items[i].setFont(m_font);
        m_items[i].setOutlineColor(sf::Color(0, 0, 60));
        m_items[i].setOutlineThickness(2.f);
    }

    m_highlight.setFillColor(sf::Color(255, 255, 255, 50));
    m_highlight.setOutlineColor(sf::Color(100, 200, 255, 180));
    m_highlight.setOutlineThickness(2.f);

    m_state = State::Main;
    m_numItems = 4; // 3 Levels + Settings

    buildStrings();
    layout();
    updateHighlight();
}

void Menu::buildStrings() {
    if (m_state == State::Main) {
        m_numItems = 4;
        m_items[0].setString("Obstacles Unleashed");
        m_items[1].setString("Arc of Chaos");
        m_items[2].setString("Waves of Danger");
        m_items[3].setString("Settings...");
    } else {
        m_numItems = 4;
        auto& s = Settings::instance();
        std::string vol = "Volume: " + std::to_string(static_cast<int>(s.getVolume())) + "%  (< >)";
        m_items[0].setString(vol);
        m_items[1].setString(s.isMuted() ? "Mute: ON" : "Mute: OFF");
        m_items[2].setString(s.isTrainOnPlay() ? "Train on My Play: ON" : "Train on My Play: OFF");
        m_items[3].setString("Back");
    }
    layout(); // update centering
}

void Menu::layout() {
    sf::Vector2f vs   = vSize(m_window);
    unsigned itemSize = static_cast<unsigned>(vs.y * 0.06f);
    itemSize          = std::max(18u, itemSize);

    float spacing = vs.y * 0.12f;
    float startY  = vs.y * 0.42f;

    for (int i = 0; i < m_numItems; ++i) {
        m_items[i].setCharacterSize(itemSize);
        sf::FloatRect b = m_items[i].getLocalBounds();
        m_items[i].setOrigin(b.width / 2.f, b.height / 2.f);
        m_items[i].setPosition(vs.x / 2.f, startY + i * spacing);
    }
}

void Menu::updateHighlight() {
    int sel = (m_state == State::Main) ? m_selectedMain : m_selectedSettings;
    for (int i = 0; i < m_numItems; ++i) {
        m_items[i].setFillColor(i == sel
            ? sf::Color(100, 220, 255)
            : sf::Color(180, 220, 255, 160));
    }

    if (m_numItems > 0 && sel < m_numItems) {
        sf::FloatRect b = m_items[sel].getGlobalBounds();
        m_highlight.setSize(sf::Vector2f(b.width + 30.f, b.height + 14.f));
        m_highlight.setPosition(b.left - 15.f, b.top - 7.f);
    }
}

void Menu::drawBackground() {
    sf::Vector2f vs = vSize(m_window);

    sf::VertexArray bg(sf::Quads, 4);
    bg[0] = sf::Vertex(sf::Vector2f(0,    0    ), sf::Color(0,  10,  50));
    bg[1] = sf::Vertex(sf::Vector2f(vs.x, 0    ), sf::Color(0,  10,  50));
    bg[2] = sf::Vertex(sf::Vector2f(vs.x, vs.y ), sf::Color(0,  80, 140));
    bg[3] = sf::Vertex(sf::Vector2f(0,    vs.y ), sf::Color(0,  80, 140));
    m_window.draw(bg);

    float hScale = vs.y / 900.f;

    sf::Text title;
    title.setFont(m_font);
    title.setCharacterSize(std::max(24u, static_cast<unsigned>(vs.y * 0.10f)));
    title.setFillColor(sf::Color(100, 220, 255));
    title.setOutlineColor(sf::Color(0, 0, 80));
    title.setOutlineThickness(3.f * hScale);
    title.setString(m_state == State::Main ? "HYDRONAUT" : "SETTINGS");
    sf::FloatRect tb = title.getLocalBounds();
    title.setOrigin(tb.width / 2.f, tb.height / 2.f);
    title.setPosition(vs.x / 2.f, vs.y * 0.20f);
    m_window.draw(title);

    if (m_state == State::Main) {
        sf::Text sub;
        sub.setFont(m_font);
        sub.setCharacterSize(std::max(16u, static_cast<unsigned>(vs.y * 0.04f)));
        sub.setFillColor(sf::Color(255, 200, 100));
        sub.setOutlineColor(sf::Color(80, 40, 0));
        sub.setOutlineThickness(2.f * hScale);
        sub.setString("Select SimulationEnvironment");
        sf::FloatRect sb = sub.getLocalBounds();
        sub.setOrigin(sb.width / 2.f, sb.height / 2.f);
        sub.setPosition(vs.x / 2.f, vs.y * 0.30f);
        m_window.draw(sub);
    }

    sf::Text hint;
    hint.setFont(m_font);
    hint.setCharacterSize(std::max(11u, static_cast<unsigned>(vs.y * 0.024f)));
    hint.setFillColor(sf::Color(120, 180, 220, 160));
    if (m_state == State::Main) {
        hint.setString("UP/DOWN: navigate     ENTER: select     ESC in-game: return");
    } else {
        hint.setString("UP/DOWN: navigate   LEFT/RIGHT: adjust   ENTER: toggle/select");
    }
    sf::FloatRect hb = hint.getLocalBounds();
    hint.setOrigin(hb.width / 2.f, hb.height / 2.f);
    hint.setPosition(vs.x / 2.f, vs.y * 0.92f);
    m_window.draw(hint);
}

int Menu::run() {
    auto& s = Settings::instance();

    while (m_window.isOpen()) {
        layout();
        updateHighlight();

        sf::Event event;
        while (m_window.pollEvent(event)) {
            if (event.type == sf::Event::Closed) return -1;
            if (event.type == sf::Event::Resized) {
                unsigned w = event.size.width;
                unsigned h = event.size.height;
                m_window.setView(sf::View(sf::FloatRect(0.f, 0.f, (float)w, (float)h)));
            }
            if (event.type == sf::Event::KeyPressed) {
                auto code = event.key.code;
                int& sel = (m_state == State::Main) ? m_selectedMain : m_selectedSettings;

                if (code == sf::Keyboard::Up) {
                    sel = (sel - 1 + m_numItems) % m_numItems;
                } else if (code == sf::Keyboard::Down) {
                    sel = (sel + 1) % m_numItems;
                } else if (code == sf::Keyboard::Escape) {
                    if (m_state == State::Settings) {
                        m_state = State::Main;
                        buildStrings();
                    } else {
                        return -1;
                    }
                } else if (code == sf::Keyboard::Left || code == sf::Keyboard::Right) {
                    if (m_state == State::Settings && sel == 0) { // Volume
                        float vol = s.getVolume();
                        vol += (code == sf::Keyboard::Right) ? 10.f : -10.f;
                        vol = std::clamp(vol, 0.f, 100.f);
                        s.setVolume(vol);
                        if (m_music) m_music->setVolume(s.isMuted() ? 0.f : vol);
                        buildStrings();
                    }
                } else if (code == sf::Keyboard::Enter) {
                    if (m_state == State::Main) {
                        if (sel < 3) return sel + 1; // SimulationEnvironment 1,2,3
                        if (sel == 3) {
                            m_state = State::Settings;
                            buildStrings();
                        }
                    } else { // Settings
                        if (sel == 1) { // Mute
                            s.setMuted(!s.isMuted());
                            if (m_music) m_music->setVolume(s.isMuted() ? 0.f : s.getVolume());
                        } else if (sel == 2) { // Train on Play
                            s.setTrainOnPlay(!s.isTrainOnPlay());
                        } else if (sel == 3) { // Back
                            m_state = State::Main;
                        }
                        buildStrings();
                    }
                }
                updateHighlight();
            }
        }

        m_window.clear();
        drawBackground();
        m_window.draw(m_highlight);
        for (int i = 0; i < m_numItems; ++i)
            m_window.draw(m_items[i]);
        m_window.display();
    }
    return -1;
}

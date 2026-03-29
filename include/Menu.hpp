#pragma once
#include <SFML/Graphics.hpp>
#include <SFML/Audio/Music.hpp>

class Menu {
public:
    explicit Menu(sf::RenderWindow& window, sf::Music* music = nullptr);

    // Returns: 1=Level1, 2=Level2, 3=Level3, -1=quit/closed
    int run();

private:
    void updateHighlight();
    void drawBackground();
    void layout();
    void buildStrings();

    sf::RenderWindow& m_window;
    sf::Music*        m_music;
    sf::Font&         m_font;

    enum class State { Main, Settings };
    State m_state = State::Main;

    // We need up to 4 items now (3 levels + settings, or 4 settings options)
    static constexpr int MAX_ITEMS = 4;
    sf::Text          m_items[MAX_ITEMS];
    sf::RectangleShape m_highlight;
    
    int m_selectedMain     = 0;
    int m_selectedSettings = 0;
    int m_numItems         = MAX_ITEMS;
};

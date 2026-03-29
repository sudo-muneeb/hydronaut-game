#include "Submarine.hpp"
#include "AssetManager.hpp"
#include "Constants.hpp"
#include "EntityState.hpp"
#include <cmath>
#include <algorithm>

static constexpr float PLAYER_SCALE_FACTOR = 0.1f;

Submarine::Submarine(sf::Vector2u windowSize) {
    m_sprite.setTexture(AssetManager::instance().texture("submarine"));
    // Start in NormalState — no conditional selection needed in the context.
    m_state = std::make_unique<NormalState>();
    reset(windowSize);
}

void Submarine::reset(sf::Vector2u windowSize) {
    auto& tex  = AssetManager::instance().texture("submarine");
    float scale = (windowSize.y * PLAYER_SCALE_FACTOR) /
                  static_cast<float>(tex.getSize().y > 0 ? tex.getSize().y : 1);
    m_sprite.setScale(scale, scale);
    m_sprite.setPosition(windowSize.x * 0.05f, windowSize.y * 0.5f);
    m_vel        = {0.f, 0.f};
    m_dashFired  = false;
    m_dashActive = false;
    m_winSize    = windowSize;
    m_state      = std::make_unique<NormalState>();    // restore to default state
}

// ─── Command interface ─────────────────────────────────────────────────────────
// Pure delegation — no conditional logic in the context.
bool Submarine::applyCommand(int action, bool dashRequested) noexcept {
    bool wasDashing = isDashing();
    m_state->handleInput(*this, action, dashRequested);
    // Returns true if we transitioned INTO a dash this frame.
    return !wasDashing && isDashing();
}

void Submarine::update(float dt) noexcept {
    m_state->update(*this, dt);
}

// ─── Draw ─────────────────────────────────────────────────────────────────────
void Submarine::draw(sf::RenderWindow& window) {
    m_particles.draw(window);

    if (isDashing()) {
        // Dynamic flash colour — state dictates logical state, draw uses it
        sf::Uint8 flash = static_cast<sf::Uint8>(128 + 127 * 0.8f); // simpler approximation
        m_sprite.setColor(sf::Color(flash, 220, 255, 220));
    } else {
        m_sprite.setColor(sf::Color::White);
    }
    window.draw(m_sprite);
}

// ─── State transition ─────────────────────────────────────────────────────────
void Submarine::changeState(std::unique_ptr<IEntityState> newState) noexcept {
    m_dashActive = (dynamic_cast<DashState*>(newState.get()) != nullptr);
    m_state = std::move(newState);
}

// ─── Hitboxes ─────────────────────────────────────────────────────────────────
sf::FloatRect Submarine::getBounds() const noexcept {
    return m_sprite.getGlobalBounds();
}

sf::FloatRect Submarine::getGrazeBounds() const noexcept {
    sf::FloatRect b = getBounds();
    b.left   -= GRAZE_INFLATE_PX;
    b.top    -= GRAZE_INFLATE_PX;
    b.width  += GRAZE_INFLATE_PX * 2.f;
    b.height += GRAZE_INFLATE_PX * 2.f;
    return b;
}

// ─── Dash state queries ────────────────────────────────────────────────────────
bool Submarine::isDashing() const noexcept { return m_dashActive; }

bool Submarine::dashAvailable() const noexcept { return isDashAvailable(); }
bool Submarine::isDashAvailable() const noexcept {
    if (!m_dashFired) return true;
    return m_dashClock.getElapsedTime().asSeconds() >= DASH_COOLDOWN_SEC;
}

float Submarine::dashCooldownRemaining() const noexcept {
    if (!m_dashFired) return 0.f;
    float el = m_dashClock.getElapsedTime().asSeconds();
    return std::max(0.f, DASH_COOLDOWN_SEC - el);
}

sf::Vector2f Submarine::getPosition() const noexcept {
    auto b = getBounds();
    return { b.left + b.width * 0.5f, b.top + b.height * 0.5f };
}

// ─── State-accessible physics ─────────────────────────────────────────────────
void Submarine::integratePosition() noexcept {
    m_sprite.move(m_vel);
}

void Submarine::clampToWindow() noexcept {
    if (m_winSize.x == 0 || m_winSize.y == 0) return;
    sf::FloatRect b  = m_sprite.getGlobalBounds();
    float ww = static_cast<float>(m_winSize.x);
    float wh = static_cast<float>(m_winSize.y);
    float px = m_sprite.getPosition().x;
    float py = m_sprite.getPosition().y;
    if (b.left < 0.f)              { px -= b.left;                   m_vel.x = 0.f; }
    if (b.left + b.width > ww)     { px -= (b.left + b.width - ww); m_vel.x = 0.f; }
    if (b.top < 0.f)               { py -= b.top;                    m_vel.y = 0.f; }
    if (b.top + b.height > wh)     { py -= (b.top + b.height - wh); m_vel.y = 0.f; }
    m_sprite.setPosition(px, py);
}

void Submarine::tickParticles(float dt) noexcept {
    sf::FloatRect b = getBounds();
    sf::Vector2f  rear(b.left, b.top + b.height * 0.5f);
    m_particles.emit(rear, m_vel, 1);
    m_particles.update(dt);
}

void Submarine::emitDashParticles(float dt) noexcept {
    sf::FloatRect b = getBounds();
    sf::Vector2f  rear(b.left, b.top + b.height * 0.5f);
    m_particles.emit(rear, m_vel, 1);
    m_particles.emit(rear, m_vel, 2);   // extra burst during dash
    m_particles.update(dt);
}

void Submarine::spawnDashTrail(int count) {
    sf::FloatRect b = getBounds();
    sf::Vector2f  rear(b.left, b.top + b.height * 0.5f);
    m_particles.emit(rear, m_vel, count);
}

void Submarine::markDashFired() noexcept {
    m_dashFired = true;
    m_dashActive = true;
    m_dashClock.restart();
}

// ─── Debug ────────────────────────────────────────────────────────────────────
void Submarine::drawDebugHitboxes(sf::RenderWindow& window) const {
    sf::FloatRect core = getBounds();
    sf::RectangleShape coreRect({core.width, core.height});
    coreRect.setPosition({core.left, core.top});
    coreRect.setFillColor(sf::Color::Transparent);
    coreRect.setOutlineColor(sf::Color(255, 60, 60, 200));
    coreRect.setOutlineThickness(2.f);
    window.draw(coreRect);

    sf::FloatRect graze = getGrazeBounds();
    sf::RectangleShape grazeRect({graze.width, graze.height});
    grazeRect.setPosition({graze.left, graze.top});
    grazeRect.setFillColor(sf::Color::Transparent);
    grazeRect.setOutlineColor(sf::Color(255, 230, 0, 160));
    grazeRect.setOutlineThickness(1.5f);
    window.draw(grazeRect);
}

// ─── Memento Pattern API ────────────────────────────────────────────────────
Submarine::Snapshot Submarine::saveState() const {
    Snapshot snap;
    snap.pos          = m_sprite.getPosition();
    snap.vel          = m_vel;
    snap.dashClockSec = m_dashClock.getElapsedTime().asSeconds();
    snap.dashFired    = m_dashFired;
    snap.dashActive   = m_dashActive;
    if (m_state) {
        snap.stateObj = m_state->clone();
    }
    return snap;
}

void Submarine::restoreState(const Snapshot& snap) {
    m_sprite.setPosition(snap.pos);
    m_vel        = snap.vel;
    // Approximating clock restore for Memento
    // If dash was fired, we just need the clock to reflect the elapsed time.
    // For SFML sf::Clock we can't set it, but we can set the boolean properly
    // and accept slight inaccuracies in dash cooldown upon reset, or just
    // manage the delta internally. For RL, the episodes usually reset to step 0.
    m_dashFired  = snap.dashFired;
    m_dashActive = snap.dashActive;
    if (snap.stateObj) {
        m_state = snap.stateObj->clone();
    }
}

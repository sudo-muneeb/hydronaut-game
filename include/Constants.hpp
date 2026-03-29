#pragma once

// ─── Physics (velocity-based movement) ───────────────────────────────────────
constexpr float PLAYER_ACCEL     = 0.8f;   // velocity added per frame per key
constexpr float PLAYER_DRAG      = 0.92f;  // water resistance multiplier (< 1.0)
constexpr float PLAYER_MAX_SPEED = 9.0f;   // terminal velocity cap

// ─── Dash ability ─────────────────────────────────────────────────────────────
constexpr int   DASH_IFRAME_FRAMES   = 10;
constexpr float DASH_VEL_MULTIPLIER  = 4.0f;
constexpr float DASH_COOLDOWN_SEC    = 3.0f;
constexpr float DASH_SHAKE_INTENSITY = 3.0f;
constexpr int   DASH_SHAKE_FRAMES    = 6;

// ─── Graze mechanic ───────────────────────────────────────────────────────────
constexpr float GRAZE_INFLATE_PX = 22.f;
constexpr int   GRAZE_SCORE_PER_FRAME = 1;

// ─── Sonar pulse ability ──────────────────────────────────────────────────────
constexpr float SONAR_EXPAND_SPEED = 220.f;
constexpr float SONAR_MAX_RADIUS   = 350.f;
constexpr float SONAR_SLOW_FACTOR  = 0.30f;
constexpr float SONAR_COOLDOWN_SEC = 5.0f;

// ─── Screen shake ─────────────────────────────────────────────────────────────
constexpr float SHAKE_INTENSITY_DEATH = 5.0f;
constexpr int   SHAKE_FRAMES_DEATH    = 10;

// ─── Hit stop ─────────────────────────────────────────────────────────────────
constexpr int HIT_STOP_MS = 500;

// ─── Gameplay tuning ──────────────────────────────────────────────────────────
constexpr int   INITIAL_OBSTACLE_SPEED  = 6;
constexpr float OBSTACLE_SPAWN_CHANCE   = 50.f;
constexpr int   SCORE_SPEED_THRESHOLD   = 500;
constexpr float SCORE_SPEED_EXPONENT    = 1.3f;

// ─── Asset paths ──────────────────────────────────────────────────────────────
constexpr const char* ASSET_FONT         = "assets/Bangers.ttf";
constexpr const char* ASSET_MUSIC        = "assets/back.mp3";

constexpr const char* ASSET_SUBMARINE    = "assets/military-sub.png";
constexpr const char* ASSET_CRAB         = "assets/black-crab.png";
constexpr const char* ASSET_FISH         = "assets/shark.png";
constexpr const char* ASSET_OCTOPUS      = "assets/octopus.png";
constexpr const char* ASSET_TREASURE     = "assets/treasurechest.png";
constexpr const char* ASSET_URCHIN       = "assets/black-urchin.png";
constexpr const char* ASSET_STARFISH     = "assets/star-fish.png";



#pragma once

#include <algorithm>

class Settings {
public:
    static Settings& instance() {
        static Settings s;
        return s;
    }

    bool isMuted() const           { return m_muted; }
    void setMuted(bool m)          { m_muted = m; }

    float getVolume() const        { return m_volume; }
    void  setVolume(float v)       { m_volume = std::clamp(v, 0.0f, 100.0f); }

    bool isTrainOnPlay() const     { return m_trainOnPlay; }
    void setTrainOnPlay(bool t)    { m_trainOnPlay = t; }

private:
    Settings() = default;

    bool  m_muted       = false;
    float m_volume      = 50.0f;
    bool  m_trainOnPlay = false;
};

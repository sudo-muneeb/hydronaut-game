#pragma once
#include <string>

// The receiver interface: strictly decoupled from sf::Keyboard or origin.
class ICommandTarget {
public:
    virtual ~ICommandTarget() = default;
    
    // 0=Up, 1=Down, 2=Left, 3=Right
    virtual void move(int baseDir) = 0;  
    
    virtual void dash() = 0;
    virtual void triggerSonar() = 0;
    virtual void idle() = 0;
};

// Abstract Command Form
class Command {
public:
    virtual ~Command() = default;
    virtual void execute(ICommandTarget& target) = 0;
    virtual std::string serialize() const = 0;
};

// --- Concrete Commands ---
class MoveCommand : public Command {
public:
    explicit MoveCommand(int dir) : m_dir(dir) {}
    void execute(ICommandTarget& target) override { target.move(m_dir); }
    std::string serialize() const override { return "MOVE:" + std::to_string(m_dir); }
private:
    int m_dir;
};

class DashCommand : public Command {
public:
    void execute(ICommandTarget& target) override { target.dash(); }
    std::string serialize() const override { return "DASH"; }
};

class SonarCommand : public Command {
public:
    void execute(ICommandTarget& target) override { target.triggerSonar(); }
    std::string serialize() const override { return "SONAR"; }
};

class IdleCommand : public Command {
public:
    void execute(ICommandTarget& target) override { target.idle(); }
    std::string serialize() const override { return "IDLE"; }
};

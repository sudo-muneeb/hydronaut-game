#pragma once
#include "Command.hpp"
#include <queue>
#include <memory>
#include <string>
#include <fstream>

class CommandQueue {
public:
    explicit CommandQueue(const std::string& telemetryLogPath);
    ~CommandQueue();

    void push(std::unique_ptr<Command> cmd);

    // Pops commands, serializes them to the log, and executes them instantly.
    void process(ICommandTarget& target);

private:
    std::queue<std::unique_ptr<Command>> m_queue;
    std::ofstream m_telemetryLog;
};

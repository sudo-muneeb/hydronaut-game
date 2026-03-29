#include "CommandQueue.hpp"
#include <iostream>

CommandQueue::CommandQueue(const std::string& telemetryLogPath) {
    if (!telemetryLogPath.empty()) {
        m_telemetryLog.open(telemetryLogPath, std::ios::out | std::ios::app);
        if (!m_telemetryLog.is_open()) {
            std::cerr << "[WARNING] Failed to open telemetry log: " << telemetryLogPath << "\n";
        }
    }
}

CommandQueue::~CommandQueue() {
    if (m_telemetryLog.is_open()) {
        m_telemetryLog.close();
    }
}

void CommandQueue::push(std::unique_ptr<Command> cmd) {
    if (cmd) {
        m_queue.push(std::move(cmd));
    }
}

void CommandQueue::process(ICommandTarget& target) {
    while (!m_queue.empty()) {
        auto& cmd = m_queue.front();

        // Telemetry Serialization
        if (m_telemetryLog.is_open()) {
            m_telemetryLog << cmd->serialize() << "\n";
            // In a real high-perf scenario, we might not flush every line,
            // but for telemetry reliability, it's safer.
            m_telemetryLog.flush();
        }

        // Execute decoupled logic
        cmd->execute(target);

        m_queue.pop();
    }
}

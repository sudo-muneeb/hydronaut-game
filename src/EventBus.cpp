#include "EventBus.hpp"
#include <algorithm>

void EventBus::subscribe(const std::string& eventType, IObserver* observer) {
    m_subscribers[eventType].push_back(observer);
}

void EventBus::unsubscribe(const std::string& eventType, IObserver* observer) {
    auto it = m_subscribers.find(eventType);
    if (it == m_subscribers.end()) return;
    auto& vec = it->second;
    vec.erase(std::remove(vec.begin(), vec.end(), observer), vec.end());
}

void EventBus::publish(const Event& event) {
    auto it = m_subscribers.find(event.type);
    if (it == m_subscribers.end()) return;
    // Copy to tolerate mid-notify unsubscription
    auto observers = it->second;
    for (IObserver* obs : observers) {
        obs->notify(event);
    }
}

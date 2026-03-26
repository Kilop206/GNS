#include "engine/core/SimulationEngine.hpp"
#include "engine/events/Event.hpp"
#include <memory>

namespace kns {

    kns::SimulationEngine::SimulationEngine() = default;

    kns::SimulationEngine::~SimulationEngine() = default;

}

void kns::SimulationEngine::run() {
    while (eventQueue.hasEvents()) {
        auto event = eventQueue.next();
        clock.tick(event->getTimestamp());
        event->execute(*this);
    }
}

void kns::SimulationEngine::schedule(kns::Event* event) {
    eventQueue.schedule(std::unique_ptr<Event>(event));
}
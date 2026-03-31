#include "engine/core/SimulationEngine.hpp"
#include "engine/events/Event.hpp"
#include <memory>

namespace kns {

	// Default constructor for the SimulationEngine class.
    kns::SimulationEngine::SimulationEngine() = default;

	// Default destructor for the SimulationEngine class.
    kns::SimulationEngine::~SimulationEngine() = default;

    // The run method of the SimulationEngine class. This method processes events from the event queue until there are no more events to process.
    void kns::SimulationEngine::run() {

        // The run method will continue to execute events until the event queue is empty, 
        // allowing the simulation to progress over time based on the scheduled events.
        while (eventQueue.hasEvents()) {

            // Retrieves the next event from the event queue. The event with the earliest timestamp will be returned first.
            auto event = eventQueue.next();

            // Advances the simulation clock to the timestamp of the event being executed.
            clock.tick(event->getTimestamp());

            // Executes the event by calling its execute method, passing a reference to the SimulationEngine.
            event->execute(*this);
        }
    }

    // The schedule method of the SimulationEngine class. This method allows users to add events to the simulation by scheduling them in the event queue.
    void kns::SimulationEngine::schedule(kns::Event* event) {
        eventQueue.schedule(std::unique_ptr<Event>(event));
    }

}
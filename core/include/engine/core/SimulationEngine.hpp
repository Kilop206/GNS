#pragma once

#include "engine/core/EventQueue.hpp"
#include "engine/time/SimulationClock.hpp"

namespace kns {

	// Forward declaration of the Event class to avoid circular dependencies. 
    // The SimulationEngine class will use pointers to Event objects, so we only need to declare the class here without including its full definition.
    class kns::Event;

    class SimulationEngine {
    public:
		// Constructor and destructor for the SimulationEngine class. 
        // The constructor initializes the simulation clock and event queue, 
        // while the destructor can be used to perform any necessary cleanup when the SimulationEngine object is destroyed.
        SimulationEngine();
        ~SimulationEngine();

		// Starts the simulation by processing events from the event queue. 
        // The run method will continue to execute events until the event queue is empty, allowing the simulation to progress over time based on the scheduled events.
        void run();

		// Schedules a new event to be processed by the simulation engine. 
        // The event will be added to the event queue, and it will be executed at the appropriate time based on its timestamp. 
        // This method allows users of the SimulationEngine to add events dynamically during the simulation.
        void schedule(Event* event);

    private:

		// The SimulationClock object that keeps track of the current simulation time. 
        // It provides methods to advance the simulation time and retrieve the current time, 
        // allowing the SimulationEngine to manage the timing of events effectively.
        kns::SimulationClock clock;

		// The EventQueue object that holds the scheduled events for the simulation. 
        // It allows the SimulationEngine to manage and process events in the correct order based on their timestamps, 
        // ensuring that the simulation progresses according to the scheduled events.
        kns::EventQueue eventQueue;
    };

}
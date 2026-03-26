#pragma once

#include "engine/core/EventQueue.hpp"
#include "engine/time/SimulationClock.hpp"

namespace kns {

    class kns::Event;

    class SimulationEngine {
    public:
        SimulationEngine();
        ~SimulationEngine();

        void run();
        void schedule(Event* event);

    private:
        kns::SimulationClock clock;
        kns::EventQueue eventQueue;
    };

}
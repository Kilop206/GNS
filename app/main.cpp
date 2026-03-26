#include <iostream>
#include "engine/time/SimulationClock.hpp"
#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PrintEvent.hpp"

int main() {

    kns::SimulationEngine engine;

    engine.schedule(new kns::PrintEvent(10, "Evento B"));
    engine.schedule(new kns::PrintEvent(5, "Evento A"));
    engine.schedule(new kns::PrintEvent(12, "Evento C"));

    engine.run();

    return 0;
}
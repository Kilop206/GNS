#include <iostream>
#include "kns/engine/SimulationClock.hpp"
#include "kns/engine/SimulationEngine.hpp"
#include "kns/engine/PrintEvent.hpp"

int main() {

    kns::SimulationEngine engine;

    engine.schedule(new kns::PrintEvent(10, "Evento B"));
    engine.schedule(new kns::PrintEvent(5, "Evento A"));
    engine.schedule(new kns::PrintEvent(12, "Evento C"));

    engine.run();

    return 0;
}
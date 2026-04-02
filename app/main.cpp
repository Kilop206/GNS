#include "core/SimulationEngine.hpp"
#include "events/PacketGenerationEvent.hpp"

using namespace kns;

int main() {

    Topology topo(100);

    SimulationEngine engine(topo);

    for (int i = 0; i < 100; i++) {
        int source = i % topo.size();
        int dest = (i + 1) % topo.size();

        auto event = std::make_unique<PacketGenerationEvent>(
            0.0,
            source,
            dest,
            1500
        );

        engine.schedule(std::move(event));
    }

    engine.run();

    engine.exportStatsCSV("results.csv");

    return 0;
}
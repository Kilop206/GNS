#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketGenerationEvent.hpp"
#include "network/TopologyLoader.hpp"

using namespace kns;

int main() {

    Topology topo = TopologyLoader::load_topology("topologies/mesh5.json");

    SimulationEngine engine(topo);

    for (int i = 0; i < 100; i++) {
        int source = i % topo.size();
        int dest = (i + 1) % topo.size();

        auto event = std::make_unique<PacketGenerationEvent>(
            i * 0.01,
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
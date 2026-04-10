#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketGenerationEvent.hpp"
#include "network/TopologyLoader.hpp"
#include "network/Topology.hpp"
#include "engine/core/RunConfig.hpp"

#include <iostream>

using namespace kns;

int main(int argc, char* argv[]) {

	if (argc < 2) {
		std::cerr << "Usage: ./kns_app <topology_file> (e.g: if you are in app, then use ./topologies/mesh.json)\n";
		return 1;
	}

	std::string path = argv[1];

    Topology topo = TopologyLoader::load_topology(path);

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

    kns::RunConfig runConfig = {"results.csv", 1};
    srand(runConfig.seed);

    engine.run();

    engine.exportStatsCSV(runConfig);

    return 0;
}

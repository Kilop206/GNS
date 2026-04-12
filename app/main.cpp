#include "engine/core/SimulationEngine.hpp"
#include "engine/events/PacketGenerationEvent.hpp"
#include "network/TopologyLoader.hpp"
#include "network/Topology.hpp"
#include "engine/core/RunConfig.hpp"
#include "network/Link.hpp"

#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <stdexcept>

using namespace kns;

int main(int argc, char* argv[]) {

    std::cout << "Starting...\n";

    if (argc < 2) {
        std::cerr << "Usage: ./kns_app <topology_file> "
                     "(e.g: ./topologies/mesh.json)\n";
        return 1;
    }

    kns::RunConfig runConfig{"results.csv", 1};

    std::string path = argv[1];

    for (int i = 1; i < argc; i++) {

        if (i + 1 >= argc) continue;

        std::string arg = argv[i];
        std::string value = argv[i + 1];

        if (arg == "--seed") {
            runConfig.seed = std::stoi(value);

        } 
        else if (arg == "--out") {
            runConfig.filename = value;
        }
        else if (arg == "--packet_size") {

            int size = std::stoi(value);

            if (size <= 0) {
                throw std::runtime_error("packet_size must be > 0");
            }

            runConfig.packet_size = size;
        }
    }

    Topology topo = TopologyLoader::load_topology(path);

    SimulationEngine engine(topo);

    for (int i = 1; i < argc; i++) {

        if (i + 1 >= argc) continue;

        std::string arg = argv[i];
        std::string value = argv[i + 1];

        if (arg == "--loss_prob") {

            double prob = std::stod(value);

            if (prob < 0.0 || prob > 1.0) {
                throw std::runtime_error(
                    "loss_prob must be between 0.0 and 1.0"
                );
            }

            auto& links = topo.getLinks();

            for (auto& row : links) {
                for (auto& link : row) {
                    link.loss_prob = prob;
                }
            }
        } 
    }

    srand(runConfig.seed);

    for (int i = 0; i < 100; i++) {
        int source = i % topo.size();
        int dest = (i + 1) % topo.size();

        auto event = std::make_unique<PacketGenerationEvent>(
            i * 0.01,
            source,
            dest,
            runConfig.packet_size
        );

        engine.schedule(std::move(event));
    }

    engine.run();

    engine.exportStatsCSV(runConfig);

    return 0;
}
#include "network/TopologyLoader.hpp"
#include "network/Topology.hpp"
#include "network/Link.hpp"

#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace kns {

    Topology TopologyLoader::load_topology(const std::string& filename) {

        std::ifstream file(filename);

        if (!file.is_open()) {
            throw std::runtime_error("Cannot open topology file");
        }

        json j;
        file >> j;

        int nodes = j["nodes"];

        Topology topology(nodes);

        for (auto& l : j["links"]) {
            Link link;

            link.from = l["from"];
            link.to = l["to"];
            link.delay_ms = l["delay"];
            link.bandwidth_mbps = l["bandwidth"];

            topology.addLink(link);
        }

        return topology;
    }

}
#include <iostream>
#include <exception>
#include "kns/network/topology.hpp"
#include "kns/network/topology_loader.hpp"

int main() {
    try {

        kns::Topology topo = kns::load_topology("mesh5.json");

        for (int i = 0; i < topo.size(); i++) {
            std::cout << "Node " << i << " neighbors:\n";

            for (const auto& link : topo.getLinksFromNode(i)) {
                std::cout
                    << " -> "
                    << link.to
                    << " delay="
                    << link.delay_ms
                    << "ms\n";
            }
        }

    }
    catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << std::endl;
        return 1;
    }
}
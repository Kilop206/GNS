#include <limits>
#include <vector>

#include "network/Node.hpp"
#include "Topology.hpp"

namespace KNS {
	class Routing {
		
		// Structure to hold the results of Dijkstra's algorithm, 
		// including the shortest distances from the source node to all other nodes and the parent nodes for path reconstruction.
		struct DijkstraResult {
			std::vector<double> dist;
			std::vector<int> parent;
		};

		// Dijkstra's algorithm to compute shortest paths from a source node to all other nodes in the topology
		DijkstraResult buildDijkstra(const KNS::Topology& topology, int src);
	};
}
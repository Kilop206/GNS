#include <limits>
#include <vector>

#include "network/Node.hpp"
#include "Topology.hpp"

namespace kns {
	class Routing {
		public:
			// Structure to hold the results of Dijkstra's algorithm, 
			// including the shortest distances from the source node to all other nodes and the parent nodes for path reconstruction.
			struct DijkstraResult {
				std::vector<double> dist;
				std::vector<int> parent;
			};

<<<<<<< HEAD
		// Dijkstra's algorithm to compute shortest paths from a source node to all other nodes in the topology
		DijkstraResult buildDijkstra(const KNS::Topology& topology, int src);

		// Builds the routing table for a given source node in the topology using Dijkstra's algorithm.
		std::vector<int> buildRoutingTable(const Topology::Topology& topology, int src);
=======
			// Dijkstra's algorithm to compute shortest paths from a source node to all other nodes in the topology
			DijkstraResult buildDijkstra(const kns::Topology& topology, int src);
>>>>>>> a6db360fc62a4dec1f1ae6f56ae6c1ebdbc06746
	};
}
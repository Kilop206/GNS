#include <limits>
#include <vector>
#include <queue>
#include <cassert>

#include "network/Routing.hpp"
#include "network/Topology.hpp"
#include "network/Link.hpp"

namespace kns {

	// Dijkstra's algorithm to compute shortest paths from a source node to all other nodes in the topology
	// Complexity: O((V + E) log V)
	Routing::DijkstraResult Routing::buildDijkstra(const Topology& topology, int src) {

		int n = topology.size();
		assert(src >= 0 && src < n);

		const double inf = std::numeric_limits<double>::infinity();

		std::vector<double> dist(n, inf);
		std::vector<int> parent(n, -1);

		std::priority_queue<
			std::pair<double, int>,
			std::vector<std::pair<double, int>>,
			std::greater<>
		> pq;

		dist[src] = 0;
		pq.push({0, src});

		while (!pq.empty()) {
			auto [currentDist, u] = pq.top();
			pq.pop();

			// Skip outdated entries (lazy deletion optimization)
			if (currentDist > dist[u]) continue;

			const auto& adjacency = topology.getLinksFromNode(u);

			for (const Link& link : adjacency) {
				int v = link.getOtherNode(u);
				double newDist = dist[u] + link.delay_ms;

				if (newDist < dist[v]) {
					dist[v] = newDist;
					parent[v] = u;
					pq.push({newDist, v});
				}
			}
		}

		return {dist, parent};
	}


	/* Builds the routing table for a given source node.
	   nextHop[d] = first node after src on the shortest path to d */
	std::vector<int> Routing::buildRoutingTable(const Topology& topology, int src) {

		int n = topology.size();
		assert(src >= 0 && src < n);

		DijkstraResult result = buildDijkstra(topology, src);

		std::vector<int> nextHop(n, -1);

		const auto& parent = result.parent;
		const auto& dist = result.dist;

		const double inf = std::numeric_limits<double>::infinity();

		for (int d = 0; d < n; ++d) {

			// No route or same node
			if (d == src || dist[d] == inf) {
				nextHop[d] = -1;
				continue;
			}

			int current = d;

			// Walk back until we reach a direct neighbor of src
			while (parent[current] != src) {

				current = parent[current];

				// Safety check (should not happen if dist[d] != inf)
				if (current == -1) {
					nextHop[d] = -1;
					break;
				}
			}

			// If valid, current is the next hop
			if (current != -1) {
				nextHop[d] = current;
			}
		}

		return nextHop;
	}

}
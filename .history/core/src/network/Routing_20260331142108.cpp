#include <limits>
#include <vector>
#include <queue>
#include <cassert>

#include "network/Routing.hpp"
#include "Topology.hpp"
#include "Link.hpp"

namespace kns {
	// Dijkstra's algorithm to compute shortest paths from a source node to all other nodes in the topology
	// Complexity: O((V + E) log V) where V is the number of nodes and E is the number of edges in the topology
	Routing::DijkstraResult kns::buildDijkstra(const kns::Topology& topology, int src) {

		int n = topology.size();

		assert(src >= 0 && src < n);

		double inf = std::numeric_limits<double>::infinity();
		std::vector<double> dist(n, inf);

		std::vector<int> parent(n, -1);
		std::priority_queue<
			std::pair<double, int>,
			std::vector<std::pair<double, int>>,
			std::greater<std::pair<double, int>>
		> pq;
		
		dist[src] = 0;
		pq.push({ 0, src });

		while (!pq.empty()) {
			auto [currentDist, u] = pq.top();
			pq.pop();
			const std::vector<Link>& adjacency_list = topology.getLinksFromNode(u);

			if (currentDist > dist[u]) continue;
			for (const Link& link : adjacency_list) {
				int v = link.getOtherNode(u);
				double new_distance = dist[u] + link.delay_ms;

				if (new_distance < dist[v]) {
					dist[v] = new_distance;
					parent[v] = u;
					pq.push({ new_distance, v });

				}
			}
		}

		return kns::Routing::DijkstraResult{ dist, parent };
	}
}
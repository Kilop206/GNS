#include <limits>
#include <vector>
#include <queue>
#include <cassert>

#include "network/Routing.hpp"
#include "network/Topology.hpp"
#include "network/Link.hpp"

namespace kns {
	// Dijkstra's algorithm to compute shortest paths from a source node to all other nodes in the topology
	// Complexity: O((V + E) log V) where V is the number of nodes and E is the number of edges in the topology
	Routing::DijkstraResult Routing::buildDijkstra(const kns::Topology& topology, int src) {

		// Get the number of nodes in the topology
		int n = topology.size();

		// Ensure the source node ID is valid
		assert(src >= 0 && src < n);

		// Initialize distances to all nodes as infinity and parent pointers as -1 (indicating no parent)
		double inf = std::numeric_limits<double>::infinity();

		// Distance array to hold the shortest distance from the source node to each node. Initialized to infinity.
		std::vector<double> dist(n, inf);

		// Parent array to reconstruct paths. parent[v] will hold the node from which we reached v in the shortest path.
		std::vector<int> parent(n, -1);

		// Priority queue to hold nodes to explore, ordered by their current shortest distance from the source node.
		std::priority_queue<
			std::pair<double, int>,
			std::vector<std::pair<double, int>>,
			std::greater<std::pair<double, int>>
		> pq;

		// Start from the source node with a distance of 0		
		dist[src] = 0;

		// Push the source node into the priority queue
		pq.push({ 0, src });

		// Main loop of Dijkstra's algorithm
		while (!pq.empty()) {

			// Get the node with the smallest distance from the priority queue
			auto [currentDist, u] = pq.top();
			pq.pop();

			// Get the list of links originating from node u
			const std::vector<Link>& adjacency_list = topology.getLinksFromNode(u);

			// If the distance we popped from the priority queue is greater than the currently known shortest distance to node u, skip processing this node
			if (currentDist > dist[u]) continue;

			// Explore each link from node u to its neighbors
			for (const Link& link : adjacency_list) {

				// Get the neighboring node v connected by this link
				int v = link.getOtherNode(u);

				// Calculate the new distance to node v through node u
				// Using link delay as routing cost
				double new_distance = dist[u] + link.delay_ms;

				// If the new distance to node v is shorter than the previously known distance, update the distance and parent information
				if (new_distance < dist[v]) {
					dist[v] = new_distance;
					parent[v] = u;
					pq.push({ new_distance, v });

				}
			}
		}
		// Return the results of Dijkstra's algorithm, including the shortest distances and parent pointers for path reconstruction
		return Routing::DijkstraResult{ dist, parent };
	}

	/* Builds the routing table for a given source node in the topology using Dijkstra's algorithm. 
		The routing table maps each destination node to the next hop node on the shortest path from the source node.*/
	std::vector<int> Routing::buildRoutingTable(const Topology& topology, int src) {

		// Get the number of nodes in the topology
		int n = topology.size();

		// Ensure the source node ID is valid
		assert(src >= 0 && src < n);

		// Run Dijkstra's algorithm to get the shortest paths and parent information from the source node
		DijkstraResult result = buildDijkstra(topology, src);

		// Initialize the routing table with -1, indicating that there is no route to the destination node
		std::vector<int> nextHop(n, -1);

		// For each destination node, determine the next hop on the shortest path from the source node
		const auto& parent = result.parent;

		// The distance array from Dijkstra's algorithm, which holds the shortest distance from the source node to each destination node
		const auto& dist = result.dist;

		// Define infinity for comparison. If the distance to a node is infinity, it means there is no path from the source to that node.
		double inf = std::numeric_limits<double>::infinity();
		for (int d = 0; d < n; d++) {

			// If the destination node is the same as the source node or if there is no path to the destination node (indicated by parent[d] == -1), set the next hop to -1
			if (d == src || dist[d] == inf) {
				nextHop[d] = -1;
				continue;
			} else {
				int currentNode = d;

				// Traverse the parent pointers from the destination node back to the source node to find the next hop. 
				// The next hop is the first node we encounter that is directly connected to the source node.
				for (int i = 0; i < n; i++) {
					int parentCurrent = parent[currentNode];
					if (parentCurrent == -1) {
						// If we reach a node with no parent, it means there is no valid path from the source to the destination. Mark as no route and stop.
						currentNode = -1;
						break;
					}
					if (parentCurrent == src) {
						// If the parent of the current node is the source node, we have found the next hop. Stop traversing.
						break;
					}
					currentNode = parentCurrent;
				}
				// Set the next hop for the destination node to the current node, which is the first node on the path from the source to the destination that is directly connected to the source.
				nextHop[d] = currentNode;
			}
		}
		return nextHop;
	}
}
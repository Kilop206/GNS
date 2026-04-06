#include "network/Topology.hpp"

namespace kns {

	// Constructor to initialize the topology with a given number of nodes
	Topology::Topology(int nodes) : num_nodes(nodes) {

		// Initialize the adjacency list with empty vectors for each node
		adjacency_list.resize(num_nodes);
	}

	// Method to add a link to the topology
	void Topology::addLink(const Link& link) {

		// Add the link to the adjacency list of the 'from' node
		adjacency_list[link.from].push_back(link);
	}

	// Method to retrieve the list of links originating from a given node
	const std::vector<Link>& kns::Topology::getLinksFromNode(int node_id) const {

		// Return the adjacency list for the specified node ID
		return adjacency_list[node_id];
	}

	// Method to get the total number of nodes in the topology
	int Topology::size() const {
		return num_nodes;

	}
}
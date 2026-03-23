#include "kns/network/topology.hpp"

namespace kns {
	Topology::Topology(int nodes) : num_nodes(nodes) {
		adjacency_list.resize(num_nodes);
	}

	void Topology::addLink(const Link& link) {
		adjacency_list[link.from].push_back(link);
	}

	const std::vector<Link>& kns::Topology::getLinksFromNode(int node_id) const
	{
		return adjacency_list[node_id];
	}

	int Topology::size() const {
		return num_nodes;

	}
}
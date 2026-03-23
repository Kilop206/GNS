#pragma once

#include <vector>
#include "kns/network/link.hpp"

namespace kns {
	class Topology {
	private:
		int num_nodes;

		std::vector<std::vector<Link>> adjacency_list;

	public:
		Topology(int num_nodes);

		void addLink(const Link& link);

		const std::vector<Link>& getLinksFromNode(int node_id) const;

		int size() const;
	};
}
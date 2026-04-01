#include "network/Link.hpp"

#include <cassert>
#include <stdexcept>

namespace kns {

	// Returns the ID of the other node connected by this link
	int Link::getOtherNode(int node) const {

		// Validate that the provided node ID is one of the two nodes connected by this link
		assert(node == from || node == to);

		// Return the other node ID
		if (node == from) {
			return to;
		}
		else {
			return from;
		}
	}

	bool Link::should_drop() const {
		return ((double) rand() / RAND_MAX) < loss_prob;
	}
}
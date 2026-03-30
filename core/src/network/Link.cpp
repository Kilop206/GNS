#include "network/Link.hpp"

#include <cassert>
#include <stdexcept>

namespace kns {
	int Link::getOtherNode(int node) const {
		assert(node == from || node == to);
		if (node == from) {
			return to;
		}
		else (node == to) {
			return from;
		}
	}
}
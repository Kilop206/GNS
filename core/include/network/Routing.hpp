#include <limits>
#include <vector>

#include "network/Node.hpp"

namespace KNS {
	class Routing {
		std::vector<double> dist(KNS::Link);
		std::vector<int> parent(KNS::Link link, -1);
		std::priority_queue<KNS::Link, std::vector<KNS::Link>, std::greater<KNS::Link>> pq;


	};
}
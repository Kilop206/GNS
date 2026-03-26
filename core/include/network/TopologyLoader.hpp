#include "network/Topology.hpp"

#include <fstream>
#include <stdexcept>
#include <nlohmann/json.hpp>

using json = nlohmann::json;

namespace kns {

	class TopologyLoader {

	public:
		static Topology load_topology(const std::string& filename);
	};

}
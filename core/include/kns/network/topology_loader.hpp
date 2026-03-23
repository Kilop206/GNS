#pragma once

#include <string>
#include "kns/network/topology.hpp"

namespace kns {
    Topology load_topology(const std::string& filename);
}
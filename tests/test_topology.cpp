#include <catch2/catch_test_macros.hpp>
#include "network/TopologyLoader.hpp"

TEST_CASE("Topology loads correctly") {
    auto topo = kns::TopologyLoader::load_topology("topologies/mesh5.json");

    REQUIRE(topo.size() == 5);
}
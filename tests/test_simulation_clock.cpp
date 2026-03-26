#include <catch2/catch_test_macros.hpp>
#include <catch2/catch_approx.hpp>
#include "engine/time/SimulationClock.hpp"

using namespace kns;

TEST_CASE("SimulationClock initializes at zero") {
    kns::SimulationClock clock;
    REQUIRE(clock.now() == 0.0);
}

TEST_CASE("SimulationClock advances correctly") {
    kns::SimulationClock clock;

    clock.tick(0.5);
    REQUIRE(clock.now() == 0.5);

    clock.tick(0.5);
    REQUIRE(clock.now() == 1.0);
}

TEST_CASE("Multiple ticks accumulate deterministically") {
    kns::SimulationClock clock;

    for (int i = 0; i < 10; ++i)
        clock.tick(0.1);

    REQUIRE(clock.now() == Catch::Approx(1.0));
}
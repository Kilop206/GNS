#include <catch2/catch_test_macros.hpp>

#include "kns/engine/EventQueue.hpp"
#include "kns/engine/Event.hpp"

class TestEvent : public kns::Event {
public:
    TestEvent(std::uint64_t t) : Event(t) {}

    void execute(kns::SimulationEngine&) override {}
};

TEST_CASE("EventQueue schedules events") {
    kns::EventQueue queue;

    queue.schedule(std::make_unique<TestEvent>(10));
    queue.schedule(std::make_unique<TestEvent>(5));

    REQUIRE(queue.size() == 2);
}

TEST_CASE("EventQueue pops earliest event first") {
    kns::EventQueue queue;

    queue.schedule(std::make_unique<TestEvent>(10));
    queue.schedule(std::make_unique<TestEvent>(5));

    auto first = queue.next();
    auto second = queue.next();

    REQUIRE(first->getTimestamp() <= second->getTimestamp());
}
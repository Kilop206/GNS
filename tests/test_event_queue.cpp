#include <catch2/catch_test_macros.hpp>
#include "core/EventQueue.h"

class TestEvent : public core::Event {
public:
    TestEvent(std::uint64_t timestamp)
        : core::Event(timestamp) {}

    void execute() override {}
};

TEST_CASE("EventQueue starts empty") {
    EventQueue queue;

    REQUIRE(queue.hasEvents() == false);
    REQUIRE(queue.size() == 0);
    REQUIRE(queue.next() == nullptr);
}

TEST_CASE("Scheduling increases size") {
    EventQueue queue;

    queue.schedule(std::make_unique<TestEvent>(10));

    REQUIRE(queue.hasEvents());
    REQUIRE(queue.size() == 1);
}

TEST_CASE("Events are ordered by timestamp") {
    EventQueue queue;

    queue.schedule(std::make_unique<TestEvent>(10));
    queue.schedule(std::make_unique<TestEvent>(5));
    queue.schedule(std::make_unique<TestEvent>(20));

    REQUIRE(queue.next()->getTimestamp() == 5);
    REQUIRE(queue.next()->getTimestamp() == 10);
    REQUIRE(queue.next()->getTimestamp() == 20);
}

TEST_CASE("Events with same timestamp are ordered by id") {
    EventQueue queue;

    queue.schedule(std::make_unique<TestEvent>(50));
    queue.schedule(std::make_unique<TestEvent>(50));

    auto first = queue.next()->getId();
    auto second = queue.next()->getId();

    REQUIRE(first < second);
}

TEST_CASE("Scheduling nullptr throws") {
    EventQueue queue;

    REQUIRE_THROWS_AS(
        queue.schedule(nullptr),
        std::invalid_argument
    );
}

TEST_CASE("Clear removes all events") {
    EventQueue queue;

    queue.schedule(std::make_unique<TestEvent>(1));
    queue.schedule(std::make_unique<TestEvent>(2));

    queue.clear();

    REQUIRE(queue.size() == 0);
    REQUIRE(queue.hasEvents() == false);
    REQUIRE(queue.next() == nullptr);
}
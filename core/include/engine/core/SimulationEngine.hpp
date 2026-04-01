#pragma once

#include <queue>
#include <memory>
#include <vector>
#include <cstdint>

#include "network/Topology.hpp"
#include "network/Routing.hpp"
#include "events/Event.hpp"

namespace kns {

    class SimulationEngine {
    public:

        // Constructor that initializes the simulation engine with a given topology.
        explicit SimulationEngine(const Topology& topology);

        // Schedules a new event to be processed by the simulation engine.
        void schedule(std::unique_ptr<Event> event);

        // Runs the simulation by processing events from the event queue.
        void run();

        // Returns the current simulation time.
        std::uint64_t now() const;

        // Returns the next hop for a packet based on the routing table.
        int getNextHop(int current, int destination) const;

        // Returns a const reference to the topology.
        const Topology& getTopology() const;

        double compute_arrival_time(const Packet& pkt, const Link& link, double now);

    private:

        // Current simulation time.
        std::uint64_t current_time_ = 0;

        // Network topology.
        Topology topology_;

        // Routing tables for each node.
        std::vector<std::vector<int>> routing_tables_;

        // Event comparison functor for priority queue.
        struct EventCompare {
            bool operator()(const std::unique_ptr<Event>& a,
                            const std::unique_ptr<Event>& b) const {
                if (a->getTimestamp() == b->getTimestamp()) {
                    return a->getId() > b->getId();
                }
                return a->getTimestamp() > b->getTimestamp();
            }
        };

        std::priority_queue<
            std::unique_ptr<Event>,
            std::vector<std::unique_ptr<Event>>,
            EventCompare
        > event_queue_;

    };

}
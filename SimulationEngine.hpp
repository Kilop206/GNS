#pragma once

#include <queue>
#include <memory>
#include <vector>
#include <cstdint>
#include <string>
#include <unordered_map>

#include "network/Topology.hpp"
#include "network/Routing.hpp"
#include "engine/events/Event.hpp"
#include "engine/core/Stats.hpp"
#include "engine/core/EventQueue.hpp"
#include "engine/time/SimulationClock.hpp"
#include "network/Packet.hpp"
#include "network/Link.hpp"
#include "engine/core/RunConfig.hpp"

namespace kns {

    class SimulationEngine {
    private:

        double loss_prob = 0.01;

        std::unordered_map<int, std::queue<Packet>> buffers;

        size_t max_queue_size = 50;

        // Current simulation time.
        SimulationClock clock_;

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

        // Statistics for the simulation
        Stats stats_;

        // Event queue for managing scheduled events
        EventQueue event_queue_;

    public:


        double random();

        double get_loss_prob() const;

        // Constructor that initializes the simulation engine with a given topology.
        explicit SimulationEngine(const Topology& topology);

        // Schedules a new event to be processed by the simulation engine.
        void schedule(std::unique_ptr<Event> event);

        // Runs the simulation by processing events from the event queue.
        void run();

        // Returns the current simulation time.
        double now() const;

        // Returns the next hop for a packet based on the routing table.
        int getNextHop(int current, int destination) const;

        // Returns a const reference to the topology.
        const Topology& getTopology() const;

        // Returns the collected statistics of the simulation.
        Stats& getStats();

        // Computes the arrival time of a packet at the next node based on the link characteristics.
        double compute_arrival_time(const Packet& pkt, const Link& link, double now);

        // Simulates sending a packet over a link, including potential packet loss and scheduling the next event for packet arrival.
        void sendPacket(const Packet& pkt, const Link& link, double now);

        // Exports the collected statistics to a CSV file for analysis.
        void exportStatsCSV(const RunConfig& runConfig);
    };

}
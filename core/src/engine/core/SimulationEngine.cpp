#include <memory>
#include <queue>
#include <cmath>
#include <iostream>

#include "core/SimulationEngine.hpp"
#include "events/Event.hpp"
#include "events/PacketReceivedEvent.hpp"
#include "network/Packet.hpp"

namespace kns {

    // Constructor that initializes the simulation engine with a given topology
    SimulationEngine::SimulationEngine(const Topology& topology)
        : topology_(topology) {
        int n = topology_.size();
        routing_tables_.resize(n);

        Routing routing;

        for (int u = 0; u < n; ++u) {
            routing_tables_[u] = routing.buildRoutingTable(topology_, u);
        }
    }

    // Schedule a new event in the simulation
    void SimulationEngine::schedule(std::unique_ptr<Event> event) {
        event_queue_.push(std::move(event));
    }

    // Get the current simulation time
    std::uint64_t SimulationEngine::now() const {
        return current_time_;
    }

    // Get the topology of the network
    const Topology& SimulationEngine::getTopology() const {
        return topology_;
    }

    // Get the next hop from the current node to the destination
    int SimulationEngine::getNextHop(int current, int destination) const {
        return routing_tables_[current][destination];
    }

    // Run the simulation until there are no more events to process
    void SimulationEngine::run() {

        while (!event_queue_.empty()) {

            // Get next event
            auto event = std::move(const_cast<std::unique_ptr<Event>&>(event_queue_.top()));
            event_queue_.pop();

            // Advance time
            current_time_ = event->getTimestamp();

            // Execute event
            event->execute(*this);
        }
    }

    // Helper function to compute the arrival time of a packet at the next node
    double compute_arrival_time(const Packet& pkt, const Link& link, double now) {
        // Transmission time = packet size (bits) / bandwidth (bits per second)
        double transmission =
            (pkt.packet_size_bytes * 8.0) / (link.bandwidth_mbps * 1e6);
        
        // Propagation time = link delay (ms) converted to seconds
        double propagation = link.delay_ms / 1000.0;

        // Total time = propagation + transmission
        return std::ceil(now + propagation + transmission);
    }

    // Send a packet over a link, simulating potential loss and scheduling the next event
    void SimulationEngine::sendPacket(
        const Packet& pkt,
        const Link& link,
        double now
    ) {
        stats_.packets_sent++;

        // Simulate packet loss
        if (link.should_drop()) {
            stats_.packets_lost++;

            std::cout << "[DROPPED] Packet from " << pkt.source
                    << " to " << pkt.destination
                    << " at time " << now
                    << std::endl;
            return;
        }

        // Compute arrival time at the next node
        double arrival_time = compute_arrival_time(pkt, link, now);

        // Determine the next node
        int next_node = link.getOtherNode(pkt.current_node);

        // Create a new packet instance for the next hop
        Packet new_pkt = pkt;
        new_pkt.current_node = next_node;
        new_pkt.hop_count++;

        // Update stats for delivered packets
        stats_.packets_delivered++;

        // Create a packet arrival event for the next node
        auto event = std::make_unique<PacketReceivedEvent>(
            arrival_time,
            new_pkt,
            next_node
        );

        // Schedule the packet arrival event
        event_queue_.push(std::move(event));

        // Update latency stats 
        stats_.total_latency += arrival_time - now;
    }
}
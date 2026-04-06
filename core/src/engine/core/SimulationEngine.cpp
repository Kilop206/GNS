#include <memory>
#include <queue>
#include <cmath>
#include <iostream>
#include <fstream>

#include "engine/core/SimulationEngine.hpp"
#include "engine/events/Event.hpp"
#include "engine/events/PacketReceivedEvent.hpp"
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
        event_queue_.schedule(std::move(event));
    }

    // Get the current simulation time
    double SimulationEngine::now() const {
        return clock_.now();
    }

    // Get the topology of the network
    const Topology& SimulationEngine::getTopology() const {
        return topology_;
    }

    // Get the collected statistics of the simulation
    Stats& SimulationEngine::getStats() {
        return stats_;
    }

    // Get the next hop from the current node to the destination
    int SimulationEngine::getNextHop(int current, int destination) const {
        return routing_tables_[current][destination];
    }

    // Run the simulation until there are no more events to process
    void SimulationEngine::run() {
        while (event_queue_.hasEvents()) {

            auto event = event_queue_.next();

            clock_.setTime(event->getTimestamp());

            event->execute(*this);
        }
    }

    // Helper function to compute the arrival time of a packet at the next node
    double SimulationEngine::compute_arrival_time(const Packet& pkt, const Link& link, double now) {

        // Transmission time = packet size (bits) / bandwidth (bits per second)
        double transmission =
            (pkt.packet_size_bytes * 8.0) / (link.bandwidth_mbps * 1e6);
        
        // Propagation time = link delay (ms) converted to seconds
        double propagation = link.delay_ms / 1000.0;

        // Total time = propagation + transmission
        return (now + propagation + transmission);
    }

    // Send a packet over a link, simulating potential loss and scheduling the next event
    void SimulationEngine::sendPacket(const Packet& pkt, const Link& link, double now) {
        stats_.packets_sent++;

        // Drop
        if (link.should_drop()) {
            stats_.packets_lost++;

            std::cout << "[DROPPED] Packet from " << pkt.source
                    << " to " << pkt.destination
                    << " at time " << now
                    << std::endl;
            return;
        }

        // Compute arrival
        double arrival_time = compute_arrival_time(pkt, link, now);

        int next_node = link.getOtherNode(pkt.current_node);

        Packet new_pkt = pkt;
        new_pkt.current_node = next_node;
        new_pkt.hop_count++;

        auto event = std::make_unique<PacketReceivedEvent>(
            arrival_time,
            new_pkt
        );

        event_queue_.schedule(std::move(event));
    }

    void SimulationEngine::exportStatsCSV(const std::string& filename) {
        // Append to the file if it exists, otherwise create a new one
        std::ofstream file(filename, std::ios::app);

        // If the file is new, write the header
        if (file.tellp() == 0) {
            file << "Packets Sent,Packets Delivered,Packets Lost,Delivery Rate,Loss Rate,Average Latency\n";
        }

        // Calculate rates and average latency
        double delivery_rate = stats_.packets_sent > 0
            ? (double)stats_.packets_delivered / stats_.packets_sent
            : 0.0;

        double loss_rate = stats_.packets_sent > 0
            ? (double)stats_.packets_lost / stats_.packets_sent
            : 0.0;
        double avg_latency = stats_.packets_delivered > 0 
                            ? stats_.total_latency / stats_.packets_delivered
                            : 0.0;

        // Write the stats to the CSV file
        file << stats_.packets_sent << ","
            << stats_.packets_delivered << ","
            << stats_.packets_lost << ","
            << delivery_rate << ","
            << loss_rate << ","
            << avg_latency << "\n";
    }
}
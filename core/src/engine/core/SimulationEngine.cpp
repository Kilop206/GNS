#include <memory>

#include "core/SimulationEngine.hpp"
#include "events/Event.hpp"
#include "network/Packet.hpp"

namespace kns {

    SimulationEngine::SimulationEngine(const Topology& topology)
        : topology_(topology)
    {
        int n = topology_.size();
        routing_tables_.resize(n);

        Routing routing;

        for (int u = 0; u < n; ++u) {
            routing_tables_[u] = routing.buildRoutingTable(topology_, u);
        }
    }

    void SimulationEngine::schedule(std::unique_ptr<Event> event) {
        event_queue_.push(std::move(event));
    }

    std::uint64_t SimulationEngine::now() const {
        return current_time_;
    }

    const Topology& SimulationEngine::getTopology() const {
        return topology_;
    }

    int SimulationEngine::getNextHop(int current, int destination) const {
        return routing_tables_[current][destination];
    }

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

    double compute_arrival_time(const Packet& pkt, const Link& link, double now) {
        double transmission_time =
            (pkt.packet_size_bytes * 8.0) / (link.bandwidth_mbps * 1e6);

        double propagation_time = link.delay_ms / 1000.0;

        return now + propagation_time + transmission_time;
    }
}
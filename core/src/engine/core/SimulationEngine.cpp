#include <memory>
#include <queue>

#include "core/SimulationEngine.hpp"
#include "events/Event.hpp"
#include "events/PacketReceivedEvent.hpp"
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

    void SimulationEngine::sendPacket(
        const Packet& pkt,
        const Link& link,
        double now
    ) {
        double arrival_time = compute_arrival_time(pkt, link, now);

        int next_node = link.getOtherNode(pkt.current_node);

        Packet new_pkt = pkt;
        new_pkt.current_node = next_node;

        auto event = std::make_unique<PacketReceivedEvent>(
            arrival_time,
            new_pkt,
            next_node
        );

        event_queue_.push(std::move(event));
    }
}
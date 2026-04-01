#include <cmath>
#include <cassert>

#include "events/PacketReceivedEvent.hpp"
#include "core/SimulationEngine.hpp"

namespace kns {

    PacketReceivedEvent::PacketReceivedEvent(std::uint64_t timestamp, Packet packet)
        : Event(timestamp), packet(std::move(packet)) {}

    void PacketReceivedEvent::execute(SimulationEngine& engine) {
        // Get the current node and destination from the packet
        int u = packet.current_node;
        int dest = packet.destination;

        // If the packet has reached its destination, we can simply return without scheduling any further events.
        if (u == dest) {
            return;
        }

        // Get the next hop for the packet from the routing table. If there is no valid next hop (i.e., getNextHop returns -1), we can return without scheduling any further events.
        int next = engine.getNextHop(u, dest);

        if (next == -1) {
            return;
        }

        // Get the link between the current node and the next hop. We need to find the link in the topology that connects node u to node next. If no such link exists, we can return without scheduling any further events.
        const auto& links = engine.getTopology().getLinksFromNode(u);

        const Link* selected_link = nullptr;

        for (const Link& link : links) {
            if (link.getOtherNode(u) == next) {
                selected_link = &link;
                break;
            }
        }

        // If no link is found, we can return without scheduling any further events.
        if (!selected_link) {
            return;
        }
        assert(selected_link->bandwidth_mbps > 0);

        // delay total
        double total_delay = engine.compute_arrival_time(packet, *selected_link, static_cast<double>(engine.now()));

        // tempo absoluto da simulação
        std::uint64_t new_time = engine.now() + static_cast<std::uint64_t>(std::ceil(total_delay));

        // Create a new PacketReceivedEvent for the next hop and schedule it in the simulation engine. We need to create a new packet that is identical to the current packet but with the current_node updated to the next hop. Then we create a new PacketReceivedEvent with the calculated new_time and the updated packet, and schedule it in the simulation engine.
        Packet nextPacket = packet;
        nextPacket.current_node = next;

        // Create the next event
        auto nextEvent = std::make_unique<PacketReceivedEvent>(new_time, nextPacket);

        // Schedule the next event in the simulation engine
        engine.schedule(std::move(nextEvent));
    }
}
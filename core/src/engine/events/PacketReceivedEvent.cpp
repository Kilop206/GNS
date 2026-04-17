#include <cmath>
#include <cassert>
#include <iostream>

#include "engine/events/PacketReceivedEvent.hpp"
#include "engine/core/SimulationEngine.hpp"

namespace kns {

    PacketReceivedEvent::PacketReceivedEvent(double timestamp, Packet packet)
        : Event(timestamp), packet(std::move(packet)) {}

    void PacketReceivedEvent::execute(SimulationEngine& engine) {
        // Verify that the packet is at the correct node
        int u = packet.current_node;
        int dest = packet.destination;

        // The assertion checks that the current node ID (u) is non-negative, which is a basic sanity check to ensure that the packet is located at a valid node in the network. In a well-formed network topology, node IDs should be non-negative integers, so this assertion helps catch any potential errors in the simulation where a packet might be assigned an invalid current node.
        assert(u >= 0);

        // If the packet has reached its destination, we can simply return without scheduling any further events, as there are no more hops to be made. This is a crucial check to prevent unnecessary processing and to ensure that the simulation accurately reflects the behavior of packets in a network.
        if (u == dest) {
            auto& stats = engine.getStats();

            stats.packets_delivered++;

            double latency = engine.now() - packet.creation_time;
            stats.total_latency += latency;

            return;
        }

        // Get the next hop for the packet from the current node to the destination. This is done by querying the routing table in the simulation engine, which provides the next node that the packet should be forwarded to in order to reach its destination. If there is no valid next hop (i.e., if getNextHop returns -1), it means that there is no route from the current node to the destination, and we can simply return without scheduling any further events, as the packet cannot be forwarded.
        int next = engine.getNextHop(u, dest);
        if (next == -1) return;

        // Get the links from the current node
        const auto& links = engine.getTopology().getLinksFromNode(u);

        // Find the link to the next hop
        const Link* selected_link = nullptr;

        for (const Link& link : links) {
            if (link.getOtherNode(u) == next) {
                selected_link = &link;
                break;
            }
        }

        // If no link is found, we can return without scheduling any further events.
        if (!selected_link) return;

        // Schedule the packet to be sent over the selected link. This involves calculating the arrival time of the packet at the next node based on the properties of the link (such as delay and bandwidth) and then scheduling a new PacketReceivedEvent for the next node at that calculated arrival time. The sendPacket method of the simulation engine is responsible for handling the logic of sending the packet, including updating its current node and scheduling the appropriate events.
        engine.sendPacket(packet, *selected_link, timestamp_);
        engine.removePacketInTransit(packet.departure_time, u, packet.destination);
    }
}
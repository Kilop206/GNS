#pragma once

#include <cstdint>

#include "network/Packet.hpp"
#include "events/Event.hpp"

namespace kns {

    class PacketReceivedEvent : public Event {
    public:
        Packet packet;

        PacketReceivedEvent(std::uint64_t timestamp, Packet packet);

        void execute(SimulationEngine& engine) override;
    };

}